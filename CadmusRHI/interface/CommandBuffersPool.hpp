#pragma once

#include <cstdint>

#include "Defs.hpp"
#include "Types.hpp"
#include "CommandBuffer.hpp"

namespace rhi
{

	struct CADMUS_RHI_API FCommandBuffersPoolDesc
	{
		EQueueFeatures QueueFeatures{EQueueFeatures::GRAPHICS};
		uint32_t InitialCapacity{1};
	};



	class CADMUS_RHI_API ICommandBuffersPool
	{
	public:
		virtual ~ICommandBuffersPool() = default;

		virtual const FCommandBuffersPoolDesc& GetDesc() const = 0;
		virtual ICommandBuffer* Acquire() = 0;
		virtual void Recycle(ICommandBuffer* CommandBuffer) = 0;
		virtual void Reset() = 0;
	};

	using ICommandsBuffersPool = ICommandBuffersPool;
}

