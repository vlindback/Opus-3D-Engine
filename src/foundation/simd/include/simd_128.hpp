
#pragma once

#include "simd_config.hpp"

#include "simd_dispatch.hpp"

#include "simd_concepts.hpp"

namespace opus3d::foundation::simd
{
	template <typename T>
	struct simd128
	{
		using traits = detail::simd_dispatch<T>;
		using reg_t  = typename detail::simd_dispatch<T>::reg_t;

		static constexpr size_t width = 16;

		alignas(width) reg_t v;

		// Default constructor (leaves uninitialized for performance)
		inline simd128() noexcept = default;

		// Implicit wrap from intrinsic type
		inline simd128(reg_t native) noexcept : v(native) {}

		// Broadcast constructor: simd128<float> a(5.0f);
		explicit inline simd128(T scalar) noexcept { v = traits::broadcast(scalar); }

		// Stores+Loads

		// Note that we have the default as unaligned.
		// It makes it harder to do wrong, and the performance concerned user can be explicit.

		static inline simd128 load(const T* ptr) noexcept { return {traits::load_unaligned(ptr)}; }

		static inline simd128 load_aligned(const T* ptr) noexcept { return {traits::load_aligned(ptr)}; }

		void inline store(T* ptr) const noexcept { traits::store_unaligned(ptr, v); }

		void inline store_aligned(T* ptr) const noexcept { traits::store_aligned(ptr, v); }

		// Set

		template <typename... Args>
		static inline simd128 set(Args... args) noexcept
			requires detail::concepts::supports_set<T, Args...>
		{
			static_assert(detail::concepts::supports_set<T, Args...>, "simd128::set(): incorrect number or type of arguments for this SIMD type");

			// Note: This is actual magic right here. Crisp clean magic unlike SFINAE and ifdef hell.
			// *chefs kiss*.

			return simd128{traits::set(args...)};
		}

		// Operators

		friend inline simd128 operator+(simd128 a, simd128 b) noexcept { return {traits::add(a.v, b.v)}; }

		friend inline simd128 operator-(simd128 a, simd128 b) noexcept { return {traits::sub(a.v, b.v)}; }

		friend inline simd128 operator*(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_mul<T>
		{
			return {traits::mul(a.v, b.v)};
		}

		// Only enable / if the trait has ::div()
		friend inline simd128 operator/(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_div<T>
		{
			return {traits::div(a.v, b.v)};
		}

		// To make sure user intent is clear, we expose bitwise operators only for integers.
		// integral types, float and double all get explicit functions.

		friend inline simd128 operator&(simd128 a, simd128 b) noexcept
			requires std::is_integral_v<T> && detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_and(a.v, b.v)};
		}

		friend inline simd128 operator|(simd128 a, simd128 b) noexcept
			requires std::is_integral_v<T> && detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_or(a.v, b.v)};
		}

		friend inline simd128 operator^(simd128 a, simd128 b) noexcept
			requires std::is_integral_v<T> && detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_xor(a.v, b.v)};
		}

		friend inline simd128 operator~(simd128 a) noexcept
			requires std::is_integral_v<T> && detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_not(a.v)};
		}

		static inline simd128 bit_and(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_and(a.v, b.v)};
		}

		static inline simd128 bit_or(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_or(a.v, b.v)};
		}

		static inline simd128 bit_xor(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_xor(a.v, b.v)};
		}

		static inline simd128 bit_not(simd128 a) noexcept
			requires detail::concepts::supports_bitwise<T>
		{
			return {traits::bit_not(a.v)};
		}

		inline simd128& operator+=(simd128 a) noexcept
		{
			v = traits::add(v, a.v);
			return *this;
		}

		inline simd128& operator-=(simd128 a) noexcept
		{
			v = traits::sub(v, a.v);
			return *this;
		}

		inline simd128& operator*=(simd128 a) noexcept
			requires detail::concepts::supports_mul<T>
		{
			v = traits::mul(v, a.v);
			return *this;
		}

		// Only enable / if the trait has ::div()
		inline simd128& operator/=(simd128 a) noexcept
			requires detail::concepts::supports_div<T>
		{
			v = traits::div(v, a.v);
			return *this;
		}

		// Special functions

		inline simd128 add_saturated(simd128 b) const noexcept
			requires detail::concepts::supports_saturation<T>
		{
			return {traits::add_s(v, b.v)};
		}

		inline simd128 sub_saturated(simd128 b) const noexcept
			requires detail::concepts::supports_saturation<T>
		{
			return {traits::sub_s(v, b.v)};
		}

		// Shuffle operations:

		template <int Imm>
		static inline simd128 shuffle(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			static_assert(Imm >= 0 && Imm <= 255);
			return {traits::shuffle<Imm>(a.v, b.v)};
		}

		static inline simd128 splat_x(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(0, 0, 0, 0)>(v.v, v.v)};
		}

		static inline simd128 splat_y(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(1, 1, 1, 1)>(v.v, v.v)};
		}

		static inline simd128 splat_z(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(2, 2, 2, 2)>(v.v, v.v)};
		}

		static inline simd128 splat_w(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(3, 3, 3, 3)>(v.v, v.v)};
		}

		static inline simd128 swap_xy(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(3, 2, 0, 1)>(v.v, v.v)};
		}

		static inline simd128 reverse(simd128 v) noexcept
			requires detail::concepts::supports_shuffle<T>
		{
			return {traits::shuffle<_MM_SHUFFLE(3, 2, 1, 0)>(v.v, v.v)};
		}

		// movemask (bit operation)

		static inline uint32_t movemask(simd128 v) noexcept
			requires detail::concepts::supports_movemask<T>
		{
			return {traits::movemask(v.v)};
		}

		// unpack lo/hi

		static inline simd128 unpack_lo(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_unpack<T>
		{
			return {traits::unpack_lo(a.v, b.v)};
		}

		static inline simd128 unpack_hi(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_unpack<T>
		{
			return {traits::unpack_hi(a.v, b.v)};
		}

		// comparisons

		static inline simd128 cmpeq(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_cmpeq<T>
		{
			return {traits::cmpeq(a.v, b.v)};
		}

		static inline simd128 cmplt(simd128 a, simd128 b) noexcept
			requires detail::concepts::supports_cmplt<T>
		{
			return {traits::cmplt(a.v, b.v)};
		}
	};

} // namespace opus3d::foundation::simd
