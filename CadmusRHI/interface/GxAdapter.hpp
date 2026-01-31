#pragma once

#include <string>

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{
	struct CADMUS_RHI_API FGxAdapterInfo
	{
		std::string Name;
		uint32_t VendorId{0};
		uint32_t DeviceId{0};
		bool bIsDiscrete{false};
		FQueueCounts QueueCounts{};

		FGxAdapterInfo() = default;

		FGxAdapterInfo(std::string InName, uint32_t InVendorId, uint32_t InDeviceId, bool bInIsDiscrete, FQueueCounts InQueueCounts)
			: Name(std::move(InName))
			, VendorId(InVendorId)
			, DeviceId(InDeviceId)
			, bIsDiscrete(bInIsDiscrete)
			, QueueCounts(InQueueCounts)
		{
		}
	};

	class CADMUS_RHI_API IGxAdapter
	{
	public:
		virtual ~IGxAdapter() = default;

		virtual const FGxAdapterInfo& GetInfo() const = 0;
	};
}
