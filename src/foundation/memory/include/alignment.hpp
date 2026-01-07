#pragma once

#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>

namespace opus3d::foundation
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_up(T value, size_t alignment) noexcept
	{
		// Ensure alignment is a power of two
		assert(alignment > 0 && (alignment & (alignment - 1)) == 0);

		const size_t mask = alignment - 1;
		return static_cast<T>((static_cast<size_t>(value) + mask) & ~mask);
	}

	// Overload for pointers
	template <typename T>
	[[nodiscard]] T* align_up(T* ptr, size_t alignment) noexcept
	{
		return reinterpret_cast<T*>(align_up(reinterpret_cast<uintptr_t>(ptr), alignment));
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_down(T value, size_t alignment) noexcept
	{
		// Ensure alignment is a power of two
		assert(alignment > 0 && (alignment & (alignment - 1)) == 0);

		const size_t mask = alignment - 1;
		return static_cast<T>(static_cast<size_t>(value) & ~mask);
	}

	template <typename T>
	[[nodiscard]] T* align_down(T* ptr, size_t alignment) noexcept
	{
		return reinterpret_cast<T*>(align_down(reinterpret_cast<uintptr_t>(ptr), alignment));
	}

} // namespace opus3d::foundation