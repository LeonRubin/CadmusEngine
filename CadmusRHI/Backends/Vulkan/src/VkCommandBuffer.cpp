#include "VkCommandBuffer.hpp"
#include "VkContext.hpp"
#include "Pipeline/VkPipeline.hpp"
#include "Helpers/VkHelper.hpp"

#include <assert.h>
#include <algorithm>

namespace
{
    void GetStagesAndAccessForLayout(VkImageLayout layout, VkPipelineStageFlags &stageMask, VkAccessFlags &accessMask)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            accessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            accessMask = 0;
            break;
        default:
            stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            break;
        }
    }

    void TransitionTextureLayout(VkCommandBuffer commandBuffer, rhi::vulkan::VkVulkanTexture *texture, VkImageLayout newLayout)
    {
        if (commandBuffer == VK_NULL_HANDLE || texture == nullptr || texture->GetImage() == VK_NULL_HANDLE)
        {
            return;
        }

        const VkImageLayout oldLayout = texture->GetCurrentLayout();
        if (oldLayout == newLayout)
        {
            return;
        }

        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkAccessFlags srcAccessMask = 0;
        GetStagesAndAccessForLayout(oldLayout, srcStageMask, srcAccessMask);

        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkAccessFlags dstAccessMask = 0;
        GetStagesAndAccessForLayout(newLayout, dstStageMask, dstAccessMask);

        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.srcAccessMask = srcAccessMask;
        imageBarrier.dstAccessMask = dstAccessMask;
        imageBarrier.oldLayout = oldLayout;
        imageBarrier.newLayout = newLayout;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = texture->GetImage();
        imageBarrier.subresourceRange = texture->GetSubresourceRange();

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStageMask,
            dstStageMask,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageBarrier);

        texture->SetCurrentLayout(newLayout);
    }

    PFN_vkCmdBeginRendering ResolveBeginRendering(VkDevice device)
    {
        auto beginRendering = reinterpret_cast<PFN_vkCmdBeginRendering>(
            vkGetDeviceProcAddr(device, "vkCmdBeginRendering"));
        if (beginRendering != nullptr)
        {
            return beginRendering;
        }

        return reinterpret_cast<PFN_vkCmdBeginRendering>(
            vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
    }

    PFN_vkCmdEndRendering ResolveEndRendering(VkDevice device)
    {
        auto endRendering = reinterpret_cast<PFN_vkCmdEndRendering>(
            vkGetDeviceProcAddr(device, "vkCmdEndRendering"));
        if (endRendering != nullptr)
        {
            return endRendering;
        }

        return reinterpret_cast<PFN_vkCmdEndRendering>(
            vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
    }
}

bool rhi::vulkan::FVulkanCommandBuffer::Begin(const rhi::FCommandBufferBeginDesc &Desc)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if (Desc.OneTimeSubmit)
    {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (Desc.SimultaneousUse)
    {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    const VkResult result = vkBeginCommandBuffer(CommandBuffer, &beginInfo);
    bRecording = (result == VK_SUCCESS);
    return bRecording;
}

bool rhi::vulkan::FVulkanCommandBuffer::End()
{
    const VkResult result = vkEndCommandBuffer(CommandBuffer);
    if (result == VK_SUCCESS)
    {
        bRecording = false;
        return true;
    }

    return false;
}

void rhi::vulkan::FVulkanCommandBuffer::BeginRenderPass(const rhi::FRenderPassDesc &Desc)
{
    assert(!bInRenderPass && "Already recording a render pass. Make sure to call EndRenderPass before beginning a new one.");

    assert(bRecording && "Command buffer must be recording before beginning a render pass.");

    if (CommandBuffer == VK_NULL_HANDLE || Device == VK_NULL_HANDLE)
    {
        return;
    }

    PFN_vkCmdBeginRendering cmdBeginRendering = ResolveBeginRendering(Device);
    if (cmdBeginRendering == nullptr)
    {
        LogRHI(rhi::RHI_LOGLEVEL_ERROR, "vkCmdBeginRendering/vkCmdBeginRenderingKHR is unavailable. Ensure dynamic rendering is enabled and function pointers are valid.");
        return;
    }

    VkRenderingAttachmentInfo colorAttachments[rhi::MAX_RENDER_TARGETS]{};
    const int numColorAttachments = Desc.GetColorAttachmentCount();
    uint32_t validColorAttachmentCount = 0;

    VkRenderingAttachmentInfo depthAttachment{};
    VkRenderingAttachmentInfo stencilAttachment{};
    VkRenderingAttachmentInfo *pDepthAttachment = nullptr;
    VkRenderingAttachmentInfo *pStencilAttachment = nullptr;

    bool bHasRenderArea = Desc.RenderAreaExtent.Width > 0 && Desc.RenderAreaExtent.Height > 0;
    uint32_t renderWidth = Desc.RenderAreaExtent.Width;
    uint32_t renderHeight = Desc.RenderAreaExtent.Height;

    auto updateRenderArea = [&](const VkVulkanTexture *texture, uint8_t mipLevel)
    {
        const FTextureCreateInfo &textureInfo = texture->GetCreateInfo();
        const uint32_t width = std::max(1u, textureInfo.Extent.Width >> mipLevel);
        const uint32_t height = std::max(1u, textureInfo.Extent.Height >> mipLevel);

        if (!bHasRenderArea)
        {
            renderWidth = width;
            renderHeight = height;
            bHasRenderArea = true;
            return;
        }

        renderWidth = std::min(renderWidth, width);
        renderHeight = std::min(renderHeight, height);
    };

    for (int i = 0; i < numColorAttachments; ++i)
    {
        const auto &attachmentDesc = Desc.ColorAttachments[i];
        auto *texture = VkContext->GetTexture(attachmentDesc.TextureHandle);
        assert(texture != nullptr && "Color attachment handle is invalid.");
        if (texture == nullptr)
        {
            continue;
        }

        if (texture->GetImage() == VK_NULL_HANDLE || texture->GetImageView() == VK_NULL_HANDLE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "BeginRenderPass skipped a color attachment because image/imageView is null.");
            continue;
        }

        TransitionTextureLayout(CommandBuffer, texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo &vkAttachment = colorAttachments[validColorAttachmentCount++];
        vkAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        vkAttachment.imageView = texture->GetImageView();
        vkAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vkAttachment.loadOp = static_cast<VkAttachmentLoadOp>(attachmentDesc.LoadOp);
        vkAttachment.storeOp = static_cast<VkAttachmentStoreOp>(attachmentDesc.StoreOp);
        vkAttachment.clearValue.color.float32[0] = attachmentDesc.ClearValue.float32[0];
        vkAttachment.clearValue.color.float32[1] = attachmentDesc.ClearValue.float32[1];
        vkAttachment.clearValue.color.float32[2] = attachmentDesc.ClearValue.float32[2];
        vkAttachment.clearValue.color.float32[3] = attachmentDesc.ClearValue.float32[3];

        updateRenderArea(texture, attachmentDesc.level);
    }

    if (Desc.DepthAttachment.DepthStencilFormat != EDepthStencilFormat::Unknown)
    {
        auto *depthTexture = VkContext->GetTexture(Desc.DepthAttachment.TextureHandle);
        assert(depthTexture != nullptr && "Depth attachment handle is invalid.");
        if (depthTexture != nullptr)
        {
            if (depthTexture->GetImage() == VK_NULL_HANDLE || depthTexture->GetImageView() == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "BeginRenderPass skipped depth attachment because image/imageView is null.");
            }
            else
            {
                depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachment.imageView = depthTexture->GetImageView();
                depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAttachment.loadOp = static_cast<VkAttachmentLoadOp>(Desc.DepthAttachment.LoadOp);
                depthAttachment.storeOp = static_cast<VkAttachmentStoreOp>(Desc.DepthAttachment.StoreOp);
                depthAttachment.clearValue.depthStencil.depth = Desc.DepthAttachment.clearDepth;
                depthAttachment.clearValue.depthStencil.stencil = Desc.DepthAttachment.clearStencil;
                pDepthAttachment = &depthAttachment;

                updateRenderArea(depthTexture, Desc.DepthAttachment.level);
            }
        }
    }

    if (Desc.StencilAttachment.DepthStencilFormat != EDepthStencilFormat::Unknown)
    {
        auto *stencilTexture = VkContext->GetTexture(Desc.StencilAttachment.TextureHandle);
        assert(stencilTexture != nullptr && "Stencil attachment handle is invalid.");
        if (stencilTexture != nullptr)
        {
            if (stencilTexture->GetImage() == VK_NULL_HANDLE || stencilTexture->GetImageView() == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "BeginRenderPass skipped stencil attachment because image/imageView is null.");
            }
            else
            {
                stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                stencilAttachment.imageView = stencilTexture->GetImageView();
                stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                stencilAttachment.loadOp = static_cast<VkAttachmentLoadOp>(Desc.StencilAttachment.LoadOp);
                stencilAttachment.storeOp = static_cast<VkAttachmentStoreOp>(Desc.StencilAttachment.StoreOp);
                stencilAttachment.clearValue.depthStencil.depth = Desc.StencilAttachment.clearDepth;
                stencilAttachment.clearValue.depthStencil.stencil = Desc.StencilAttachment.clearStencil;
                pStencilAttachment = &stencilAttachment;

                updateRenderArea(stencilTexture, Desc.StencilAttachment.level);
            }
        }
    }

    if (validColorAttachmentCount == 0 && pDepthAttachment == nullptr && pStencilAttachment == nullptr)
    {
        LogRHI(rhi::RHI_LOGLEVEL_ERROR, "BeginRenderPass requires at least one valid color/depth/stencil attachment.");
        return;
    }

    assert(bHasRenderArea && "BeginRenderPass requires at least one valid attachment to determine render area.");
    if (!bHasRenderArea)
    {
        return;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {
        static_cast<int32_t>(Desc.RenderAreaOffset.Width),
        static_cast<int32_t>(Desc.RenderAreaOffset.Height)};
    renderingInfo.renderArea.extent = {renderWidth, renderHeight};
    renderingInfo.layerCount = std::max(1u, Desc.layerCount);
    renderingInfo.viewMask = Desc.viewMask == 0xFFFFFFFFu ? 0u : Desc.viewMask;
    renderingInfo.colorAttachmentCount = validColorAttachmentCount;
    renderingInfo.pColorAttachments = validColorAttachmentCount > 0 ? colorAttachments : nullptr;
    renderingInfo.pDepthAttachment = pDepthAttachment;
    renderingInfo.pStencilAttachment = pStencilAttachment;

    cmdBeginRendering(CommandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = static_cast<float>(renderingInfo.renderArea.offset.x);
    viewport.y = static_cast<float>(renderingInfo.renderArea.offset.y);
    viewport.width = static_cast<float>(renderingInfo.renderArea.extent.width);
    viewport.height = static_cast<float>(renderingInfo.renderArea.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = renderingInfo.renderArea.offset;
    scissor.extent = renderingInfo.renderArea.extent;
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

    bInRenderPass = true;
    CurrentRenderPassDesc = Desc;
}

void rhi::vulkan::FVulkanCommandBuffer::EndRenderPass()
{
    assert(bInRenderPass && "Can't end a render pass without calling BeginRenderPass first");

    PFN_vkCmdEndRendering cmdEndRendering = ResolveEndRendering(Device);
    if (cmdEndRendering == nullptr)
    {
        LogRHI(rhi::RHI_LOGLEVEL_ERROR, "vkCmdEndRendering/vkCmdEndRenderingKHR is unavailable.");
        bInRenderPass = false;
        CurrentRenderPassDesc = {};
        return;
    }

    cmdEndRendering(CommandBuffer);

    const int colorAttachmentCount = CurrentRenderPassDesc.GetColorAttachmentCount();
    for (int i = 0; i < colorAttachmentCount; ++i)
    {
        auto *texture = VkContext->GetTexture(CurrentRenderPassDesc.ColorAttachments[i].TextureHandle);
        if (texture != nullptr && texture->IsSwapchainImage())
        {
            TransitionTextureLayout(CommandBuffer, texture, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }
    }

    bInRenderPass = false;
    CurrentRenderPassDesc = {};
}

void rhi::vulkan::FVulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    assert(bInRenderPass && "Draw calls must be made within a render pass.");
    vkCmdDraw(CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void rhi::vulkan::FVulkanCommandBuffer::BindPipeline(rhi::IPipeline *Pipeline)
{
    assert(bRecording && "Command buffer must be recording before binding a pipeline.");
    assert(Pipeline != nullptr && "BindPipeline requires a valid pipeline.");
    if (!bRecording || Pipeline == nullptr)
    {
        return;
    }

    auto *vkPipeline = dynamic_cast<rhi::VulkanPipeline *>(Pipeline);
    assert(vkPipeline != nullptr && "BindPipeline received a non-Vulkan pipeline implementation.");
    if (vkPipeline == nullptr)
    {
        return;
    }

    const VkPipelineBindPoint bindPoint =
        vkPipeline->GetType() == rhi::EPipelineType::Compute
            ? VK_PIPELINE_BIND_POINT_COMPUTE
            : VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkCmdBindPipeline(CommandBuffer, bindPoint, vkPipeline->GetVkPipelineHandle());
}

bool rhi::vulkan::FVulkanCommandBuffer::Reset()
{
    const VkResult result = vkResetCommandBuffer(CommandBuffer, 0);
    if (result == VK_SUCCESS)
    {
        bRecording = false;
        return true;
    }

    return false;
}
