#pragma once

#include <cstdint>
#include <source_location>

namespace opus3d::foundation
{
#ifdef FOUNDATION_DEBUG
	struct SourceLocation
	{
		constexpr SourceLocation() noexcept : m_location(std::source_location::current()) {}

		SourceLocation(const SourceLocation&) = default;

		static constexpr SourceLocation current() { return SourceLocation(); }
		constexpr const char*		file_name() const { return m_location.file_name(); }
		constexpr const char*		function_name() const noexcept { return m_location.function_name(); }
		constexpr std::uint_least32_t	line() const noexcept { return m_location.line(); }
		constexpr std::uint_least32_t	column() const noexcept { return m_location.column(); }

	private:

		std::source_location m_location;
	};
#else
	// A dummy type that occupies zero space in Release
	struct SourceLocation
	{
		static constexpr SourceLocation current() { return {}; }
		constexpr const char*		file_name() const { return ""; }
		constexpr const char*		function_name() const noexcept { return ""; }
		constexpr std::uint_least32_t	line() const noexcept { return 0; }
		constexpr std::uint_least32_t	column() const noexcept { return 0; }
	};
#endif
} // namespace opus3d::foundation