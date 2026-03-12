#pragma once

#include <Pipeline.hpp>
#include <vulkan/vulkan_core.h>
#include <optional>
#include <vector>

namespace rhi
{
    struct VkShaderStageInfo
    {
        EShaderStage ShaderStage = EShaderStage::Vertex;
        const char* EntrypointName = nullptr;
        const char* ShaderBinaryData = nullptr; // Contains either SPIRV/DXIL bytecode or GLSL/HLSL source code, depending on the value of ShaderLanguage
        size_t ShaderBinarySize = 0;
        bool bBuilderOwnsBinaryData = false;
        EShaderLanguage ShaderLanguage = EShaderLanguage::NONE;
    };
    
    class VkPipelineBuilder : public rhi::IPipelineBuilder
    {
    public:
        virtual ~VkPipelineBuilder();

        VkPipelineBuilder(VkDevice InDevice) : Device(InDevice) {};

        IPipelineBuilder& SetType(EPipelineType Type) override;
        IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* ShaderPath) override;
        IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* Bytecode, size_t BytecodeSize) override;
        IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, EShaderLanguage ShaderLanguage, const char* ShaderCode, size_t ShaderCodeSize) override;

        IPipelineBuilder& SetInputAssembly(ETopology Topology) override;
        IPipelineBuilder& SetRasterizationState(const FRasterizationStateDesc& Desc) override;
        IPipelineBuilder& SetMultisampleState(const FMultisampleStateDesc& Desc) override;
        IPipelineBuilder& SetDepthStencilState(const FDepthStencilStateDesc& Desc) override;
        IPipelineBuilder& SetColorBlendState(const FColorBlendStateDesc& Desc) override;
        IPipelineBuilder& SetOutput(size_t Index, EColorFormat Format) override;
        IPipelineBuilder& SetDepthOutput(EDepthStencilFormat Format) override;
        IPipelineBuilder& SetStencilOutput(EDepthStencilFormat Format) override;
        IPipelineBuilder& SetDynamicState(const EDynamicState* DynamicStates, size_t DynamicStateCount) override;
        IPipelineBuilder& SetVertexInputState(const FVertexInputStateDesc& Desc) override;
        IPipelineBuilder& Reset() override;
        IPipeline* Build() override;

    private:

        VkDevice Device;

        EPipelineType PipelineType{EPipelineType::Graphics};
        VkShaderStageInfo ShaderStageInfos[EShaderStage::Count];

        std::optional<VkPipelineVertexInputStateCreateInfo> VertexInputState{};
        std::optional<VkPipelineInputAssemblyStateCreateInfo> InputAssembly{};
        std::optional<VkPipelineRasterizationStateCreateInfo> RasterizationState{};
        std::optional<VkPipelineMultisampleStateCreateInfo> MultisampleState{};
        std::optional<VkPipelineDepthStencilStateCreateInfo> DepthStencilState{};
        std::optional<VkPipelineColorBlendStateCreateInfo> ColorBlendState{};
        std::optional<VkPipelineDynamicStateCreateInfo> DynamicState{};

        std::vector<VkFormat> ColorAttachmentFormats;
        VkFormat DepthAttachmentFormat{VK_FORMAT_UNDEFINED};
        VkFormat StencilAttachmentFormat{VK_FORMAT_UNDEFINED};
    };
}