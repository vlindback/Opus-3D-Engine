#pragma once

#include <foundation/core/include/assert.hpp>

#include <cstdint>
#include <string_view>

namespace opus3d::foundation::fs
{
	// UTF8 Path View
	class PathView8
	{
	public:

		constexpr PathView8() noexcept = default;

		// From a null-terminated char8_t array (literals)
		template <size_t N>
		constexpr PathView8(const char8_t (&str)[N]) noexcept : m_data(str), m_size(static_cast<uint32_t>(N - 1))
		{
			static_assert(N > 0, "String literal must be null-terminated");
		}

		// From std::u8string_view
		explicit constexpr PathView8(std::u8string_view sv) noexcept : m_data(sv.data()), m_size(static_cast<uint32_t>(sv.size())) {}

		constexpr PathView8(const char8_t* str, uint32_t len) noexcept : m_data(str), m_size(len)
		{
			// Must be null terminated to be valid OS path.
			DEBUG_ASSERT(str != nullptr && str[len] == u8'\0');
		}

		constexpr bool empty() const noexcept { return m_size == 0; }

		constexpr const char8_t* begin() const noexcept { return m_data; }
		constexpr const char8_t* end() const noexcept { return m_data + m_size; }

		constexpr const char8_t* data() const noexcept { return m_data; }

		uint32_t size() const noexcept { return m_size; }

		constexpr bool operator==(PathView8 other) const noexcept
		{
			return std::u8string_view(m_data, m_size) == std::u8string_view(other.m_data, other.m_size);
		}

		// For OS APIs.
		const char* c_str() const noexcept { return reinterpret_cast<const char*>(m_data); }

	private:

		const char8_t* m_data = nullptr;
		uint32_t       m_size = 0;
	};
} // namespace opus3d::foundation::fs