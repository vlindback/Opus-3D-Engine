#pragma once

#include <foundation/core/include/error_domain.hpp>

namespace opus3d::foundation
{
	enum class ContainerErrorCode : uint32_t
	{
		Unknown,
		ContainerFull,
	};

	// Defined in headers as inline constexpr so every TU sees the same address:

	namespace error_domains
	{
		size_t container_error_formatter(uint32_t code, std::span<char> strBuffer) noexcept;

		inline constexpr ErrorDomain Container = {"Container", container_error_formatter};
	} // namespace error_domains
}