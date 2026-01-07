#pragma once

#include <foundation/core/include/error_domain.hpp>

#include <span>

namespace opus3d::foundation::memory
{
	enum class MemoryErrorCode : uint32_t
	{
		Unknown,
		OutOfMemory,
		AllocatorNoResize,
	};
} // namespace opus3d::foundation::memory

namespace opus3d::foundation::error_domains
{
	// Defined in headers as inline constexpr so every TU sees the same address:
	size_t memory_error_formatter(uint32_t code, std::span<char> strBuffer);

	inline constexpr ErrorDomain Memory = {"Memory", memory_error_formatter};
} // namespace opus3d::foundation::error_domains