#pragma once

#include <cstdint>

#include "Defs.hpp"
#include "BitmaskTemplates.hpp"
namespace rhi
{
    enum class EQueueFeatures : uint32_t
    {
        None = 0,
        GRAPHICS = 1u << 0,
        COMPUTE = 1u << 1,
        TRANSFER = 1u << 2,
        PRESENT = 1u << 3,
        SPARSE_BINDING = 1u << 4,
        PROTECTED = 1u << 5,
        VIDEO_DECODE = 1u << 6,
        VIDEO_ENCODE = 1u << 7,
        OPTICAL_FLOW = 1u << 8,
        DATA_GRAPH_ARM = 1u << 9,
        MAX_ENUM = 10,
        Any = 0xFFFFFFFF
    };

    consteval void enable_bitmask_operators(EQueueFeatures);

    struct CADMUS_RHI_API FExtent2D
    {
        uint32_t Width{0};
        uint32_t Height{0};

        constexpr FExtent2D() = default;
        constexpr FExtent2D(uint32_t InWidth, uint32_t InHeight)
            : Width(InWidth)
            , Height(InHeight)
        {
        }
    };

    enum class EColorFormat
    {
        Unknown,
        RGBA8_UNorm,
        BGRA8_UNorm
    };

    enum class EDepthStencilFormat
    {
        Unknown,
        D32_SFloat,
        D24_UNorm_S8_UInt,
        D32_SFloat_S8_UInt
    };

    struct CADMUS_RHI_API FQueueCounts
    {
        uint32_t Graphics{0};
        uint32_t Compute{0};
        uint32_t Transfer{0};
        uint32_t Present{0};

        constexpr FQueueCounts() = default;
        constexpr FQueueCounts(uint32_t InGraphics, uint32_t InCompute, uint32_t InTransfer, uint32_t InPresent)
            : Graphics(InGraphics)
            , Compute(InCompute)
            , Transfer(InTransfer)
            , Present(InPresent)
        {
        }
    };

    enum EShaderStage
    {
        Vertex = 0,
        Fragment = 1,
        Compute = 2,
        Geometry = 3,
        TessellationControl = 4,
        TessellationEvaluation = 5,
        Mesh = 6,
        Task = 7,
        RayGeneration = 8,
        Intersection = 9,
        AnyHit = 10,
        ClosestHit = 11,
        Miss = 12,
        Callable = 13,
        Count = 14,
        Max = Count - 1
    };

    enum EShaderLanguage
    {
        NONE,
        GLSL = 0,
        HLSL = 1,
        SPIRV = 2
    };

    enum ETopology
    {
        PointList = 0,
        LineList = 1,
        LineStrip = 2,
        TriangleList = 3,
        TriangleStrip = 4,
        TriangleFan = 5
    };

    enum EPolygonMode
    {
        Fill = 0,
        Line = 1,
        Point = 2
    };

    enum ECullMode
    {
        None = 0,
        Front = 1,
        Back = 2,
        FrontAndBack = 3
    };

    enum EFrontFace
    {
        CounterClockwise = 0,
        Clockwise = 1
    };

    struct FRasterizationStateDesc
    {
        bool DepthClampEnable{false};
        bool RasterizerDiscardEnable{false};
        EPolygonMode PolygonMode{EPolygonMode::Fill};
        ECullMode CullMode{ECullMode::Back};
        EFrontFace FrontFace{EFrontFace::CounterClockwise};
        bool DepthBiasEnable{false};
        float DepthBiasConstantFactor{0.0f};
        float DepthBiasClamp{0.0f};
        float DepthBiasSlopeFactor{0.0f};
    };

    struct FMultisampleStateDesc
    {
        uint32_t RasterizationSamples{1};
        bool SampleShadingEnable{false};
        float MinSampleShading{1.0f};
        uint32_t SampleMask{0xFFFFFFFF};
        bool AlphaToCoverageEnable{false};
        bool AlphaToOneEnable{false};

        constexpr FMultisampleStateDesc() = default;
    };

        enum class EPipelineType : uint8_t
    {
        Graphics = 0,
        Compute = 1,
    };

    struct CADMUS_RHI_API FEntrypointDesc
    {
        EShaderStage Stage{EShaderStage::Vertex};
        const char* Name{nullptr};
        const char* ByteCode{nullptr};
        size_t ByteCodeSize{0};
    };

    enum class ECompareOp : uint8_t
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessOrEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterOrEqual = 6,
        Always = 7
    };

    enum class EStencilOp : uint8_t
    {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrementAndClamp = 3,
        DecrementAndClamp = 4,
        Invert = 5,
        IncrementAndWrap = 6,
        DecrementAndWrap = 7
    };

    struct CADMUS_RHI_API FStencilOpStateDesc
    {
        EStencilOp FailOp{EStencilOp::Keep};
        EStencilOp PassOp{EStencilOp::Keep};
        EStencilOp DepthFailOp{EStencilOp::Keep};
        ECompareOp CompareOp{ECompareOp::Always};
        uint32_t CompareMask{0xFFFFFFFFu};
        uint32_t WriteMask{0xFFFFFFFFu};
        uint32_t Reference{0};
    };

    struct CADMUS_RHI_API FDepthStencilStateDesc
    {
        bool DepthTestEnable{false};
        bool DepthWriteEnable{false};
        ECompareOp DepthCompareOp{ECompareOp::Less};
        bool DepthBoundsTestEnable{false};
        bool StencilTestEnable{false};
        FStencilOpStateDesc Front{};
        FStencilOpStateDesc Back{};
        float MinDepthBounds{0.0f};
        float MaxDepthBounds{1.0f};
    };

    enum class EBlendFactor : uint8_t
    {
        ZeroFactor = 0,
        OneFactor = 1,
        SrcColor = 2,
        OneMinusSrcColor = 3,
        DstColor = 4,
        OneMinusDstColor = 5,
        SrcAlpha = 6,
        OneMinusSrcAlpha = 7,
        DstAlpha = 8,
        OneMinusDstAlpha = 9
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

    enum class EBlendOp : uint8_t
    {
        Add = 0,
        Subtract = 1,
        ReverseSubtract = 2,
        Min = 3,
        Max = 4
    };

    enum class EColorComponentFlags : uint32_t
    {
        None = 0u,
        R = 1u << 0,
        G = 1u << 1,
        B = 1u << 2,
        A = 1u << 3,
        RGBA = static_cast<uint32_t>(R) | static_cast<uint32_t>(G) | static_cast<uint32_t>(B) | static_cast<uint32_t>(A)
    };
    consteval void enable_bitmask_operators(EColorComponentFlags);


    enum class ELoadOp : uint8_t
    {
        Load = 0,
        Clear = 1,
        DontCare = 2
    };

    enum class EStoreOp : uint8_t
    {
        Store = 0,
        DontCare = 1
    };

    enum class EResolveMode : uint8_t
    {
        Average = 0,
        Min = 1,
        Max = 2,
        SampleZero = 3
    };

    union UClearColorValue
    {
        float float32[4];
        int int32[4];
        unsigned int uint32[4];
    };

    enum
    {
        MAX_RENDER_TARGETS = 8
    };


    struct CADMUS_RHI_API FFrameBuffer final
    {

    };

    // Rethink to be fillable via functions instead of direct filling? (heavily inspired by LVK)
    struct CADMUS_RHI_API FDynamicRenderPass final
    {
        struct RenderTarget
        {
            ELoadOp LoadOp{ELoadOp::Load};
            EStoreOp StoreOp{EStoreOp::Store};
            EColorFormat Format{EColorFormat::Unknown};
            EDepthStencilFormat DepthStencilFormat{EDepthStencilFormat::Unknown};

            uint8_t layer = 0; // For array textures or 3D textures, specifies the layer to render to. Ignored for non-array 2D textures.
            uint8_t level = 0; // don't remember

            UClearColorValue ClearValue{.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}; // Used if LoadOp is Clear
            float clearDepth{1.0f};
            uint32_t clearStencil{0};
        };

        RenderTarget ColorAttachments[MAX_RENDER_TARGETS];
        RenderTarget DepthAttachment = {.LoadOp = ELoadOp::DontCare, .StoreOp = EStoreOp::DontCare};
        RenderTarget StencilAttachment = {.LoadOp = ELoadOp::DontCare, .StoreOp = EStoreOp::DontCare};

        uint32_t layerCount{1};
        uint32_t viewMask{0xFFFFFFFFu}; // For multiview rendering, specifies which views to render to. Each bit corresponds to a view index.

        FORCE_INLINE int GetColorAttachmentCount() const
        {
            int count = 0;
            for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
            {
                if (ColorAttachments[i].Format != EColorFormat::Unknown)
                {
                    ++count;
                }
                else
                {
                    break; // Assume that valid attachments are contiguous from the start
                }
            }
            return count;
        }
    };
}
