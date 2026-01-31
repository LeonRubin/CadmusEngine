#pragma once

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{
	struct CADMUS_RHI_API FSwapchainDesc
	{
		FExtent2D Extent{};
		uint32_t BufferCount{2};
		EColorFormat Format{EColorFormat::BGRA8_UNorm};
		bool VSync{true};

		constexpr FSwapchainDesc() = default;
		constexpr FSwapchainDesc(FExtent2D InExtent, uint32_t InBufferCount, EColorFormat InFormat, bool bInVSync)
			: Extent(InExtent)
			, BufferCount(InBufferCount)
			, Format(InFormat)
			, VSync(bInVSync)
		{
		}
	};

	class CADMUS_RHI_API ISwapchain
	{
	public:
		virtual ~ISwapchain() = default;

		virtual const FSwapchainDesc& GetDesc() const = 0;
	};
}
