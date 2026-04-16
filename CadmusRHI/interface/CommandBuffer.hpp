#pragma once

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{
    struct CADMUS_RHI_API FCommandBufferBeginDesc
	{
		bool OneTimeSubmit{false};
		bool SimultaneousUse{false};
	};

	class IPipeline;
	class CADMUS_RHI_API ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer() = default;

		virtual bool Begin(const FCommandBufferBeginDesc& Desc = {}) = 0;
		virtual bool End() = 0;
		virtual bool Reset() = 0;

		virtual void BeginRenderPass(const FRenderPassDesc& Desc) = 0;
		virtual void EndRenderPass() = 0;
		
		virtual void BindPipeline(IPipeline* Pipeline) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;

		virtual bool IsRecording() const = 0;
	};
}