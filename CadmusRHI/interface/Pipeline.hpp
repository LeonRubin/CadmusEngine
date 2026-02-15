#pragma once

#include <cstddef>

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{
    class IPipeline;

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
        const FColorBlendAttachmentStateDesc* Attachments{nullptr};
        size_t AttachmentCount{0};
        float BlendConstants[4]{0.0f, 0.0f, 0.0f, 0.0f};
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

    enum class EVertexFormat : uint8_t
    {
        Float = 0,
        Float2 = 1,
        Float3 = 2,
        Float4 = 3,
        UInt = 4,
        UInt2 = 5,
        UInt3 = 6,
        UInt4 = 7,
        SInt = 8,
        SInt2 = 9,
        SInt3 = 10,
        SInt4 = 11,
        UByte4Norm = 12
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

    class CADMUS_RHI_API IPipelineBuilder
    {
    public:
        virtual ~IPipelineBuilder() = default;
        IPipelineBuilder() = default;

        virtual IPipelineBuilder& SetType(EPipelineType Type) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* ShaderPath) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, const char* Bytecode, size_t BytecodeSize) = 0;
        virtual IPipelineBuilder& SetShaderStage(EShaderStage ShaderStage, const char* EntrypointName, EShaderLanguage ShaderLanguage, const char* ShaderCode, size_t ShaderCodeSize) = 0;

        virtual IPipelineBuilder& SetInputAssembly(ETopology Topology) = 0;
        virtual IPipelineBuilder& SetRasterizationState(const FRasterizationStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetMultisampleState(const FMultisampleStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetDepthStencilState(const FDepthStencilStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetColorBlendState(const FColorBlendStateDesc& Desc) = 0;
        virtual IPipelineBuilder& SetDynamicState(const EDynamicState* DynamicStates, size_t DynamicStateCount) = 0;
        virtual IPipelineBuilder& SetVertexInputState(const FVertexInputStateDesc& Desc) = 0;
        virtual IPipelineBuilder& Reset() = 0;
        virtual IPipeline* Build() = 0;
    };
}
