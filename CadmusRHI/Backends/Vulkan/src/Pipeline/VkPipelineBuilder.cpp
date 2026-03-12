#include "VkPipelineBuilder.hpp"

#include "Helpers/IOUtils.hpp"
#include "Helpers/VkHelper.hpp"
#include <assert.h>
#include "VkPipeline.hpp"
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>

namespace rhi
{
    namespace
    {
        VkDynamicState ConvertDynamicState(EDynamicState State)
        {
            switch (State)
            {
            case EDynamicState::Viewport:
                return VK_DYNAMIC_STATE_VIEWPORT;
            case EDynamicState::Scissor:
                return VK_DYNAMIC_STATE_SCISSOR;
            case EDynamicState::LineWidth:
                return VK_DYNAMIC_STATE_LINE_WIDTH;
            case EDynamicState::DepthBias:
                return VK_DYNAMIC_STATE_DEPTH_BIAS;
            case EDynamicState::BlendConstants:
                return VK_DYNAMIC_STATE_BLEND_CONSTANTS;
            case EDynamicState::DepthBounds:
                return VK_DYNAMIC_STATE_DEPTH_BOUNDS;
            case EDynamicState::StencilCompareMask:
                return VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
            case EDynamicState::StencilWriteMask:
                return VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
            case EDynamicState::StencilReference:
                return VK_DYNAMIC_STATE_STENCIL_REFERENCE;
            default:
                assert(false && "Unsupported dynamic state");
                return VK_DYNAMIC_STATE_MAX_ENUM;
            }
        }

        VkShaderStageFlagBits ConvertShaderStage(EShaderStage Stage)
        {
            switch (Stage)
            {
            case EShaderStage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case EShaderStage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case EShaderStage::Compute:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            case EShaderStage::Geometry:
                return VK_SHADER_STAGE_GEOMETRY_BIT;
            case EShaderStage::TessellationControl:
                return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case EShaderStage::TessellationEvaluation:
                return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case EShaderStage::Mesh:
                return VK_SHADER_STAGE_MESH_BIT_EXT;
            case EShaderStage::Task:
                return VK_SHADER_STAGE_TASK_BIT_EXT;
            case EShaderStage::RayGeneration:
                return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case EShaderStage::Intersection:
                return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case EShaderStage::AnyHit:
                return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case EShaderStage::ClosestHit:
                return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case EShaderStage::Miss:
                return VK_SHADER_STAGE_MISS_BIT_KHR;
            case EShaderStage::Callable:
                return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            default:
                assert(false && "Unsupported shader stage");
                return VK_SHADER_STAGE_ALL;
            }
        }

        shaderc_shader_kind ConvertShaderKind(EShaderStage Stage)
        {
            switch (Stage)
            {
            case EShaderStage::Vertex:
                return shaderc_glsl_vertex_shader;
            case EShaderStage::Fragment:
                return shaderc_glsl_fragment_shader;
            case EShaderStage::Compute:
                return shaderc_glsl_compute_shader;
            case EShaderStage::Geometry:
                return shaderc_glsl_geometry_shader;
            case EShaderStage::TessellationControl:
                return shaderc_glsl_tess_control_shader;
            case EShaderStage::TessellationEvaluation:
                return shaderc_glsl_tess_evaluation_shader;
            default:
                return shaderc_glsl_infer_from_source;
            }
        }

        bool CompileToSpirv(const VkShaderStageInfo &StageInfo, std::vector<uint32_t> *OutSpirv)
        {
            if (!OutSpirv)
            {
                return false;
            }

            shaderc::Compiler compiler;
            shaderc::CompileOptions options;
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);

            if (StageInfo.ShaderLanguage == EShaderLanguage::HLSL)
            {
                options.SetSourceLanguage(shaderc_source_language_hlsl);
            }
            else
            {
                options.SetSourceLanguage(shaderc_source_language_glsl);
            }

            const char *EntryPoint = StageInfo.EntrypointName ? StageInfo.EntrypointName : "main";
            const auto result = compiler.CompileGlslToSpv(
                StageInfo.ShaderBinaryData,
                StageInfo.ShaderBinarySize,
                ConvertShaderKind(StageInfo.ShaderStage),
                "CadmusPipelineShader",
                EntryPoint,
                options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                const std::string errorMessage = result.GetErrorMessage();
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, errorMessage.empty() ? "shaderc compilation failed" : errorMessage.c_str());
                return false;
            }

            OutSpirv->assign(result.cbegin(), result.cend());
            return true;
        }

        VkFormat ConvertColorFormat(EColorFormat Format)
        {
            switch (Format)
            {
            case EColorFormat::RGBA8_UNorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case EColorFormat::BGRA8_UNorm:
                return VK_FORMAT_B8G8R8A8_UNORM;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }

        VkFormat ConvertDepthStencilFormat(EDepthStencilFormat Format)
        {
            switch (Format)
            {
            case EDepthStencilFormat::D32_SFloat:
                return VK_FORMAT_D32_SFLOAT;
            case EDepthStencilFormat::D24_UNorm_S8_UInt:
                return VK_FORMAT_D24_UNORM_S8_UINT;
            case EDepthStencilFormat::D32_SFloat_S8_UInt:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }
    }

    VkPipelineBuilder::~VkPipelineBuilder()
    {
        Reset();
    }

    IPipelineBuilder &VkPipelineBuilder::SetType(EPipelineType Type)
    {
        PipelineType = Type;
        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetShaderStage(EShaderStage ShaderStage, const char *EntrypointName, const char *ShaderPath)
    {
        auto &stage = ShaderStageInfos[ShaderStage];

        stage.ShaderStage = ShaderStage;
        stage.EntrypointName = EntrypointName;

        FString extension = io::GetExtension(ShaderPath);

        if (extension == "spv")
        {
            stage.ShaderLanguage = EShaderLanguage::SPIRV;
        }
        else if (extension == "glsl")
        {
            stage.ShaderLanguage = EShaderLanguage::GLSL;
        }
        else if (extension == "hlsl")
        {
            stage.ShaderLanguage = EShaderLanguage::HLSL;
        }
        else
        {
            vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Unknown shader extension for shader stage");
            assert(false);
            return *this;
        }

        // Might be better to defer to build
        if (stage.bBuilderOwnsBinaryData)
        {
            delete[] stage.ShaderBinaryData;
            stage.ShaderBinaryData = nullptr;
        }

        stage.bBuilderOwnsBinaryData = true;
        char *shaderData = nullptr;
        size_t fileSize = 0;
        io::ReadFile(ShaderPath, stage.ShaderLanguage == EShaderLanguage::SPIRV, &shaderData, &fileSize);

        stage.ShaderBinaryData = shaderData;
        stage.ShaderBinarySize = fileSize;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetShaderStage(EShaderStage ShaderStage, const char *EntrypointName, const char *Bytecode, size_t BytecodeSize)
    {
        auto &stage = ShaderStageInfos[ShaderStage];

        stage.ShaderStage = ShaderStage;
        stage.EntrypointName = EntrypointName;

        if (stage.bBuilderOwnsBinaryData)
        {
            delete[] stage.ShaderBinaryData;
            stage.ShaderBinaryData = nullptr;
        }

        stage.bBuilderOwnsBinaryData = false;
        stage.ShaderBinaryData = Bytecode;
        stage.ShaderBinarySize = BytecodeSize;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetShaderStage(EShaderStage ShaderStage, const char *EntrypointName, EShaderLanguage ShaderLanguage, const char *ShaderCode, size_t ShaderCodeSize)
    {
        auto &stage = ShaderStageInfos[ShaderStage];

        stage.ShaderStage = ShaderStage;
        stage.EntrypointName = EntrypointName;
        stage.ShaderLanguage = ShaderLanguage;

        if (stage.bBuilderOwnsBinaryData)
        {
            delete[] stage.ShaderBinaryData;
            stage.ShaderBinaryData = nullptr;
        }

        stage.bBuilderOwnsBinaryData = false;
        stage.ShaderBinaryData = ShaderCode;
        stage.ShaderBinarySize = ShaderCodeSize;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetInputAssembly(ETopology Topology)
    {
        VkPipelineInputAssemblyStateCreateInfo newValue{};

        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        newValue.topology = vulkan::ConvertTopology(Topology);
        newValue.primitiveRestartEnable = VK_FALSE;
        InputAssembly = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetRasterizationState(const FRasterizationStateDesc &Desc)
    {
        VkPipelineRasterizationStateCreateInfo newValue{};
        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        newValue.depthClampEnable = Desc.DepthClampEnable ? VK_TRUE : VK_FALSE;
        newValue.rasterizerDiscardEnable = Desc.RasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
        newValue.polygonMode = vulkan::ConvertPolygonMode(Desc.PolygonMode);
        newValue.lineWidth = 1.0f;
        newValue.cullMode = vulkan::ConvertCullMode(Desc.CullMode);
        newValue.frontFace = Desc.FrontFace == EFrontFace::CounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        newValue.depthBiasEnable = Desc.DepthBiasEnable ? VK_TRUE : VK_FALSE;
        newValue.depthBiasConstantFactor = Desc.DepthBiasConstantFactor;
        newValue.depthBiasClamp = Desc.DepthBiasClamp;
        newValue.depthBiasSlopeFactor = Desc.DepthBiasSlopeFactor;
        RasterizationState = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetMultisampleState(const FMultisampleStateDesc &Desc)
    {
        VkPipelineMultisampleStateCreateInfo newValue{};

        if (MultisampleState.has_value())
        {
            if(MultisampleState.value().pSampleMask)
            {
                delete MultisampleState->pSampleMask;
                MultisampleState->pSampleMask = nullptr;
            }
        }

        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        newValue.rasterizationSamples = vulkan::ConvertSampleCount(Desc.RasterizationSamples);
        newValue.sampleShadingEnable = Desc.SampleShadingEnable ? VK_TRUE : VK_FALSE;
        newValue.minSampleShading = Desc.MinSampleShading;
        newValue.pSampleMask = new VkSampleMask(Desc.SampleMask);
        newValue.alphaToCoverageEnable = Desc.AlphaToCoverageEnable ? VK_TRUE : VK_FALSE;
        newValue.alphaToOneEnable = Desc.AlphaToOneEnable ? VK_TRUE : VK_FALSE;
        MultisampleState = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetDepthStencilState(const FDepthStencilStateDesc &Desc)
    {
        VkPipelineDepthStencilStateCreateInfo newValue{};
        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        newValue.depthTestEnable = Desc.DepthTestEnable ? VK_TRUE : VK_FALSE;
        newValue.depthWriteEnable = Desc.DepthWriteEnable ? VK_TRUE : VK_FALSE;
        newValue.depthCompareOp = vulkan::ConvertCompareOp(Desc.DepthCompareOp);
        newValue.depthBoundsTestEnable = Desc.DepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
        newValue.stencilTestEnable = Desc.StencilTestEnable ? VK_TRUE : VK_FALSE;

        newValue.front.failOp = vulkan::ConvertStencilOp(Desc.Front.FailOp);
        newValue.front.passOp = vulkan::ConvertStencilOp(Desc.Front.PassOp);
        newValue.front.depthFailOp = vulkan::ConvertStencilOp(Desc.Front.DepthFailOp);
        newValue.front.compareOp = vulkan::ConvertCompareOp(Desc.Front.CompareOp);
        newValue.front.compareMask = Desc.Front.CompareMask;
        newValue.front.writeMask = Desc.Front.WriteMask;
        newValue.front.reference = Desc.Front.Reference;

        newValue.back.failOp = vulkan::ConvertStencilOp(Desc.Back.FailOp);
        newValue.back.passOp = vulkan::ConvertStencilOp(Desc.Back.PassOp);
        newValue.back.depthFailOp = vulkan::ConvertStencilOp(Desc.Back.DepthFailOp);
        newValue.back.compareOp = vulkan::ConvertCompareOp(Desc.Back.CompareOp);
        newValue.back.compareMask = Desc.Back.CompareMask;
        newValue.back.writeMask = Desc.Back.WriteMask;
        newValue.back.reference = Desc.Back.Reference;

        newValue.minDepthBounds = Desc.MinDepthBounds;
        newValue.maxDepthBounds = Desc.MaxDepthBounds;

        DepthStencilState = newValue;
        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetColorBlendState(const FColorBlendStateDesc &Desc)
    {
        VkPipelineColorBlendStateCreateInfo newValue{};

        if (ColorBlendState.has_value() && ColorBlendState->pAttachments)
        {
            delete[] ColorBlendState->pAttachments;
            ColorBlendState->pAttachments = nullptr;
        }

        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        newValue.logicOpEnable = VK_FALSE;
        newValue.logicOp = VK_LOGIC_OP_COPY;
        newValue.attachmentCount = static_cast<uint32_t>(Desc.Attachments.size());

        VkPipelineColorBlendAttachmentState *Attachments = nullptr;
        if (!Desc.Attachments.empty())
        {
            Attachments = new VkPipelineColorBlendAttachmentState[Desc.Attachments.size()];
            for (size_t i = 0; i < Desc.Attachments.size(); ++i)
            {
                const auto &Src = Desc.Attachments[i];
                auto &Dst = Attachments[i];
                Dst.blendEnable = Src.BlendEnable ? VK_TRUE : VK_FALSE;
                Dst.srcColorBlendFactor = vulkan::ConvertBlendFactor(Src.SrcColorBlendFactor);
                Dst.dstColorBlendFactor = vulkan::ConvertBlendFactor(Src.DstColorBlendFactor);
                Dst.colorBlendOp = vulkan::ConvertBlendOp(Src.ColorBlendOp);
                Dst.srcAlphaBlendFactor = vulkan::ConvertBlendFactor(Src.SrcAlphaBlendFactor);
                Dst.dstAlphaBlendFactor = vulkan::ConvertBlendFactor(Src.DstAlphaBlendFactor);
                Dst.alphaBlendOp = vulkan::ConvertBlendOp(Src.AlphaBlendOp);
                Dst.colorWriteMask = vulkan::ConvertColorComponentFlags(Src.ColorWriteMask);
            }
        }

        newValue.pAttachments = Attachments;
        newValue.blendConstants[0] = Desc.BlendConstants[0];
        newValue.blendConstants[1] = Desc.BlendConstants[1];
        newValue.blendConstants[2] = Desc.BlendConstants[2];
        newValue.blendConstants[3] = Desc.BlendConstants[3];
        ColorBlendState = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetDynamicState(const EDynamicState *DynamicStates, size_t DynamicStateCount)
    {
        VkPipelineDynamicStateCreateInfo newValue{};

        if (DynamicState.has_value() && DynamicState->pDynamicStates)
        {
            delete[] DynamicState->pDynamicStates;
            DynamicState->pDynamicStates = nullptr;
        }

        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        newValue.dynamicStateCount = 0;

        VkDynamicState *States = nullptr;
        if (DynamicStates && DynamicStateCount > 0)
        {
            States = new VkDynamicState[DynamicStateCount];
            for (size_t i = 0; i < DynamicStateCount; ++i)
            {
                States[i] = ConvertDynamicState(DynamicStates[i]);
            }
            newValue.dynamicStateCount = static_cast<uint32_t>(DynamicStateCount);
        }

        newValue.pDynamicStates = States;
        DynamicState = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetOutput(size_t Index, EColorFormat Format)
    {
        if (ColorAttachmentFormats.size() <= Index)
        {
            ColorAttachmentFormats.resize(Index + 1, VK_FORMAT_UNDEFINED);
        }

        ColorAttachmentFormats[Index] = ConvertColorFormat(Format);
        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetDepthOutput(EDepthStencilFormat Format)
    {
        DepthAttachmentFormat = ConvertDepthStencilFormat(Format);
        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetStencilOutput(EDepthStencilFormat Format)
    {
        StencilAttachmentFormat = ConvertDepthStencilFormat(Format);
        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::SetVertexInputState(const FVertexInputStateDesc &Desc)
    {
        VkPipelineVertexInputStateCreateInfo newValue{};

        if (VertexInputState.has_value() && VertexInputState->pVertexAttributeDescriptions)
        {
            delete[] VertexInputState->pVertexAttributeDescriptions;
            VertexInputState->pVertexAttributeDescriptions = nullptr;
        }

        if (VertexInputState.has_value() && VertexInputState->pVertexBindingDescriptions)
        {
            delete[] VertexInputState->pVertexBindingDescriptions;
            VertexInputState->pVertexBindingDescriptions = nullptr;
        }

        newValue.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        newValue.vertexBindingDescriptionCount = static_cast<uint32_t>(Desc.BindingCount);
        VkVertexInputBindingDescription *bindingDescriptions = new VkVertexInputBindingDescription[Desc.BindingCount];
        newValue.pVertexBindingDescriptions = bindingDescriptions;
        for (size_t i = 0; i < Desc.BindingCount; i++)
        {
            const auto &srcBinding = Desc.Bindings[i];
            auto &dstBinding = bindingDescriptions[i];
            dstBinding.binding = srcBinding.Binding;
            dstBinding.stride = srcBinding.Stride;
            dstBinding.inputRate = static_cast<VkVertexInputRate>(srcBinding.InputRate);
        }

        VkVertexInputAttributeDescription *attribDecriptions = new VkVertexInputAttributeDescription[Desc.AttributeCount];
        newValue.vertexAttributeDescriptionCount = static_cast<uint32_t>(Desc.AttributeCount);
        newValue.pVertexAttributeDescriptions = attribDecriptions;
        for (size_t i = 0; i < Desc.AttributeCount; i++)
        {
            const auto &srcAttr = Desc.Attributes[i];
            auto &dstAttr = attribDecriptions[i];
            dstAttr.location = srcAttr.Location;
            dstAttr.binding = srcAttr.Binding;
            dstAttr.format = vulkan::ConvertVertexFormat(srcAttr.Format);
            dstAttr.offset = srcAttr.Offset;
        }

        VertexInputState = newValue;

        return *this;
    }

    IPipelineBuilder &VkPipelineBuilder::Reset()
    {
        for (int i = 0; i < EShaderStage::Count; ++i)
        {
            auto &Stage = ShaderStageInfos[i];
            if (Stage.bBuilderOwnsBinaryData && Stage.ShaderBinaryData)
            {
                delete[] Stage.ShaderBinaryData;
            }

            Stage = {};
        }

        if (VertexInputState.has_value() && VertexInputState->pVertexAttributeDescriptions)
        {
            delete[] VertexInputState->pVertexAttributeDescriptions;
            VertexInputState->pVertexAttributeDescriptions = nullptr;
        }

        if (VertexInputState.has_value() && VertexInputState->pVertexBindingDescriptions)
        {
            delete[] VertexInputState->pVertexBindingDescriptions;
            VertexInputState->pVertexBindingDescriptions = nullptr;
        }

        if (MultisampleState.has_value() && MultisampleState->pSampleMask)
        {
            delete MultisampleState->pSampleMask;
            MultisampleState->pSampleMask = nullptr;
        }

        if (ColorBlendState.has_value() && ColorBlendState->pAttachments)
        {
            delete[] ColorBlendState->pAttachments;
            ColorBlendState->pAttachments = nullptr;
        }

        if (DynamicState.has_value() && DynamicState->pDynamicStates)
        {
            delete[] DynamicState->pDynamicStates;
            DynamicState->pDynamicStates = nullptr;
        }

        PipelineType = EPipelineType::Graphics;
        VertexInputState = {};
        InputAssembly = {};
        RasterizationState = {};
        MultisampleState = {};
        DepthStencilState = {};
        ColorBlendState = {};
        DynamicState = {};
        ColorAttachmentFormats.clear();
        DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
        StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        return *this;
    }

    IPipeline *VkPipelineBuilder::Build()
    {
        if (Device == VK_NULL_HANDLE)
        {
            vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Cannot build pipeline: invalid Vulkan device");
            return nullptr;
        }

        std::vector<VkShaderModule> shaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<std::vector<uint32_t>> compiledSpirvModules;

        for (int i = 0; i < EShaderStage::Count; ++i)
        {
            const auto &StageInfo = ShaderStageInfos[i];
            if (!StageInfo.ShaderBinaryData || StageInfo.ShaderBinarySize == 0)
            {
                continue;
            }

            const uint32_t *spirvData = reinterpret_cast<const uint32_t *>(StageInfo.ShaderBinaryData);
            size_t spirvSize = StageInfo.ShaderBinarySize;

            if (StageInfo.ShaderLanguage == EShaderLanguage::GLSL || StageInfo.ShaderLanguage == EShaderLanguage::HLSL)
            {
                compiledSpirvModules.emplace_back();
                if (!CompileToSpirv(StageInfo, &compiledSpirvModules.back()))
                {
                    for (VkShaderModule Module : shaderModules)
                    {
                        vkDestroyShaderModule(Device, Module, nullptr);
                    }
                    return nullptr;
                }

                spirvData = compiledSpirvModules.back().data();
                spirvSize = compiledSpirvModules.back().size() * sizeof(uint32_t);
            }

            if ((spirvSize % 4) != 0)
            {
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Invalid SPIR-V binary size (must be 4-byte aligned)");
                for (VkShaderModule Module : shaderModules)
                {
                    vkDestroyShaderModule(Device, Module, nullptr);
                }
                return nullptr;
            }

            VkShaderModuleCreateInfo ShaderModuleCreateInfo{};
            ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ShaderModuleCreateInfo.codeSize = spirvSize;
            ShaderModuleCreateInfo.pCode = spirvData;

            VkShaderModule Module = VK_NULL_HANDLE;
            if (vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &Module) != VK_SUCCESS)
            {
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to create Vulkan shader module");
                for (VkShaderModule CreatedModule : shaderModules)
                {
                    vkDestroyShaderModule(Device, CreatedModule, nullptr);
                }
                return nullptr;
            }

            shaderModules.push_back(Module);

            VkPipelineShaderStageCreateInfo StageCreateInfo{};
            StageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            StageCreateInfo.stage = ConvertShaderStage(StageInfo.ShaderStage);
            StageCreateInfo.module = Module;
            StageCreateInfo.pName = StageInfo.EntrypointName ? StageInfo.EntrypointName : "main";
            shaderStages.push_back(StageCreateInfo);
        }

        if (shaderStages.empty())
        {
            vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Cannot build pipeline: no shader stages configured");
            return nullptr;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        if (vkCreatePipelineLayout(Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to create pipeline layout");
            for (VkShaderModule Module : shaderModules)
            {
                vkDestroyShaderModule(Device, Module, nullptr);
            }
            return nullptr;
        }

        VkPipeline PipelineHandle = VK_NULL_HANDLE;

        if (PipelineType == EPipelineType::Compute)
        {
            const VkPipelineShaderStageCreateInfo *ComputeStage = nullptr;
            for (const auto &Stage : shaderStages)
            {
                if (Stage.stage == VK_SHADER_STAGE_COMPUTE_BIT)
                {
                    ComputeStage = &Stage;
                    break;
                }
            }

            if (!ComputeStage)
            {
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Compute pipeline requires a compute shader stage");
                for (VkShaderModule Module : shaderModules)
                {
                    vkDestroyShaderModule(Device, Module, nullptr);
                }
                vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
                return nullptr;
            }

            VkComputePipelineCreateInfo ComputeCreateInfo{};
            ComputeCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            ComputeCreateInfo.layout = pipelineLayout;
            ComputeCreateInfo.stage = *ComputeStage;

            if (vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputeCreateInfo, nullptr, &PipelineHandle) != VK_SUCCESS)
            {
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to create compute pipeline");
                for (VkShaderModule Module : shaderModules)
                {
                    vkDestroyShaderModule(Device, Module, nullptr);
                }
                vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
                return nullptr;
            }
        }
        else
        {
            VkGraphicsPipelineCreateInfo GraphicsCreateInfo{};
            GraphicsCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            GraphicsCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            GraphicsCreateInfo.pStages = shaderStages.data();
            VkPipelineVertexInputStateCreateInfo DefaultVertexInputState{};
            DefaultVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            VkPipelineInputAssemblyStateCreateInfo DefaultInputAssemblyState{};
            DefaultInputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            DefaultInputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            DefaultInputAssemblyState.primitiveRestartEnable = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo DefaultRasterizationState{};
            DefaultRasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            DefaultRasterizationState.depthClampEnable = VK_FALSE;
            DefaultRasterizationState.rasterizerDiscardEnable = VK_FALSE;
            DefaultRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
            DefaultRasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
            DefaultRasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            DefaultRasterizationState.depthBiasEnable = VK_FALSE;
            DefaultRasterizationState.lineWidth = 1.0f;

            VkViewport DefaultViewport{};
            DefaultViewport.x = 0.0f;
            DefaultViewport.y = 0.0f;
            DefaultViewport.width = 1.0f;
            DefaultViewport.height = 1.0f;
            DefaultViewport.minDepth = 0.0f;
            DefaultViewport.maxDepth = 1.0f;

            VkRect2D DefaultScissor{};
            DefaultScissor.offset = {0, 0};
            DefaultScissor.extent = {1, 1};

            VkPipelineViewportStateCreateInfo DefaultViewportState{};
            DefaultViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            DefaultViewportState.viewportCount = 1;
            DefaultViewportState.pViewports = &DefaultViewport;
            DefaultViewportState.scissorCount = 1;
            DefaultViewportState.pScissors = &DefaultScissor;

            VkSampleMask DefaultSampleMask = 0xFFFFFFFFu;
            VkPipelineMultisampleStateCreateInfo DefaultMultisampleState{};
            DefaultMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            DefaultMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            DefaultMultisampleState.sampleShadingEnable = VK_FALSE;
            DefaultMultisampleState.minSampleShading = 1.0f;
            DefaultMultisampleState.pSampleMask = &DefaultSampleMask;
            DefaultMultisampleState.alphaToCoverageEnable = VK_FALSE;
            DefaultMultisampleState.alphaToOneEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo DefaultColorBlendState{};
            DefaultColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            DefaultColorBlendState.logicOpEnable = VK_FALSE;
            DefaultColorBlendState.logicOp = VK_LOGIC_OP_COPY;
            
            VkPipelineColorBlendAttachmentState DefaultColorBlendAttachment{};
            DefaultColorBlendAttachment.blendEnable = VK_FALSE;
            DefaultColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            DefaultColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            DefaultColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            DefaultColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            DefaultColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            DefaultColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            DefaultColorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            DefaultColorBlendState.attachmentCount = 1;
            DefaultColorBlendState.pAttachments = &DefaultColorBlendAttachment;

            GraphicsCreateInfo.pVertexInputState = VertexInputState ? &(*VertexInputState) : &DefaultVertexInputState;
            GraphicsCreateInfo.pInputAssemblyState = InputAssembly ? &(*InputAssembly) : &DefaultInputAssemblyState;
            GraphicsCreateInfo.pViewportState = &DefaultViewportState;
            GraphicsCreateInfo.pRasterizationState = RasterizationState ? &(*RasterizationState) : &DefaultRasterizationState;
            GraphicsCreateInfo.pMultisampleState = MultisampleState ? &(*MultisampleState) : &DefaultMultisampleState;
            GraphicsCreateInfo.pDepthStencilState = DepthStencilState ? &(*DepthStencilState) : nullptr;
            GraphicsCreateInfo.pColorBlendState = ColorBlendState ? &(*ColorBlendState) : &DefaultColorBlendState;
            GraphicsCreateInfo.pDynamicState = DynamicState ? &(*DynamicState) : nullptr;
            GraphicsCreateInfo.layout = pipelineLayout;

            const uint32_t RenderingColorAttachmentCount = GraphicsCreateInfo.pColorBlendState ? GraphicsCreateInfo.pColorBlendState->attachmentCount : 0;
            std::vector<VkFormat> RenderingColorAttachmentFormats;
            if (RenderingColorAttachmentCount > 0)
            {
                RenderingColorAttachmentFormats.assign(RenderingColorAttachmentCount, VK_FORMAT_B8G8R8A8_UNORM);
                for (size_t i = 0; i < ColorAttachmentFormats.size() && i < RenderingColorAttachmentFormats.size(); ++i)
                {
                    RenderingColorAttachmentFormats[i] = ColorAttachmentFormats[i];
                }
            }

            VkPipelineRenderingCreateInfo PipelineRenderingInfo{};
            PipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            PipelineRenderingInfo.colorAttachmentCount = RenderingColorAttachmentCount;
            PipelineRenderingInfo.pColorAttachmentFormats = RenderingColorAttachmentCount > 0 ? RenderingColorAttachmentFormats.data() : nullptr;
            PipelineRenderingInfo.depthAttachmentFormat = DepthAttachmentFormat;
            PipelineRenderingInfo.stencilAttachmentFormat = StencilAttachmentFormat;

            GraphicsCreateInfo.pNext = &PipelineRenderingInfo;
            GraphicsCreateInfo.renderPass = VK_NULL_HANDLE;
            GraphicsCreateInfo.subpass = 0;

            if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsCreateInfo, nullptr, &PipelineHandle) != VK_SUCCESS)
            {
                vulkan::LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to create graphics pipeline");
                for (VkShaderModule Module : shaderModules)
                {
                    vkDestroyShaderModule(Device, Module, nullptr);
                }
                
                vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
                return nullptr;
            }
        }

        for (VkShaderModule Module : shaderModules)
        {
            vkDestroyShaderModule(Device, Module, nullptr);
        }

        return new VulkanPipeline(PipelineType, Device, pipelineLayout, PipelineHandle);
    }
} // namespace rhi
