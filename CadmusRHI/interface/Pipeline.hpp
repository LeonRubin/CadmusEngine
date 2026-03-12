#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{

    struct CADMUS_RHI_API FColorBlendAttachmentStateDesc
    {
        bool BlendEnable{false};
        EBlendFactor SrcColorBlendFactor{EBlendFactor::OneFactor};
        EBlendFactor DstColorBlendFactor{EBlendFactor::ZeroFactor};
        EBlendOp ColorBlendOp{EBlendOp::Add};
        EBlendFactor SrcAlphaBlendFactor{EBlendFactor::OneFactor};
        EBlendFactor DstAlphaBlendFactor{EBlendFactor::ZeroFactor};
        EBlendOp AlphaBlendOp{EBlendOp::Add};
        EColorComponentFlags ColorWriteMask{EColorComponentFlags::RGBA};
    };

    struct CADMUS_RHI_API FColorBlendStateDesc
    {
        float BlendConstants[4]{0.0f, 0.0f, 0.0f, 0.0f};
        std::vector<FColorBlendAttachmentStateDesc> Attachments;

        FColorBlendStateDesc() = default;

        FColorBlendStateDesc(std::initializer_list<float> InBlendConstants,
                             std::initializer_list<FColorBlendAttachmentStateDesc> InAttachments)
            : Attachments(InAttachments)
        {
            const size_t count = std::min(InBlendConstants.size(), static_cast<size_t>(4));
            auto sourceIt = InBlendConstants.begin();
            for (size_t i = 0; i < count; ++i, ++sourceIt)
            {
                BlendConstants[i] = *sourceIt;
            }
        }

        FColorBlendStateDesc(const std::array<float, 4>& InBlendConstants,
                             std::initializer_list<FColorBlendAttachmentStateDesc> InAttachments)
            : Attachments(InAttachments)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                BlendConstants[i] = InBlendConstants[i];
            }
        }
    };

    enum class EDynamicState : uint8_t
    {
        Viewport = 0,
        Scissor = 1,
        LineWidth = 2,
        DepthBias = 3,
        BlendConstants = 4,
        DepthBounds = 5,
        StencilCompareMask = 6,
        StencilWriteMask = 7,
        StencilReference = 8
    };

    enum class EVertexInputRate : uint8_t
    {
        Vertex = 0,
        Instance = 1
    };

    struct CADMUS_RHI_API FVertexInputBindingDesc
    {
        uint32_t Binding{0};
        uint32_t Stride{0};
        EVertexInputRate InputRate{EVertexInputRate::Vertex};
    };

    struct CADMUS_RHI_API FVertexInputAttributeDesc
    {
        uint32_t Location{0};
        uint32_t Binding{0};
        EVertexFormat Format{EVertexFormat::Float3};
        uint32_t Offset{0};
    };

    struct CADMUS_RHI_API FVertexInputStateDesc
    {
        const FVertexInputBindingDesc* Bindings{nullptr};
        size_t BindingCount{0};
        const FVertexInputAttributeDesc* Attributes{nullptr};
        size_t AttributeCount{0};
    };

    class CADMUS_RHI_API IPipeline
    {
    public:
        virtual ~IPipeline() = default;

        virtual EPipelineType GetType() const = 0;
    };

    // Deferred builder for graphics/compute pipelines. Performs compilation/creation on Build.
    class CADMUS_RHI_API IPipelineBuilder
    {
    public:
        virtual ~IPipelineBuilder() = default;

        virtual IPipelineBuilder& SetType(EPipelineType Type) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* ShaderPath) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* Bytecode, size_t BytecodeSize) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, EShaderLanguage ShaderLanguage, const char* ShaderCode, size_t ShaderCodeSize) = 0;

        virtual IPipelineBuilder& SetInputAssembly(ETopology Topology) = 0;
        virtual IPipelineBuilder& SetRasterizationState(const FRasterizationStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetMultisampleState(const FMultisampleStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetDepthStencilState(const FDepthStencilStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetColorBlendState(const FColorBlendStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetOutput(size_t Index, EColorFormat Format) = 0;
        virtual IPipelineBuilder& SetDepthOutput(EDepthStencilFormat Format) = 0;
        virtual IPipelineBuilder& SetStencilOutput(EDepthStencilFormat Format) = 0;
        virtual IPipelineBuilder& SetDynamicState(const EDynamicState* DynamicStates, size_t DynamicStateCount) = 0;
        virtual IPipelineBuilder& SetVertexInputState(const FVertexInputStateDesc& Desc) = 0;
        virtual IPipelineBuilder& Reset() = 0;
        virtual IPipeline* Build() = 0;
    };
}
