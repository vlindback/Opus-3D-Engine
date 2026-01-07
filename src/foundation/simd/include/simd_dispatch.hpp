#pragma once

#include "simd_config.hpp"

namespace opus3d::foundation::simd::detail
{
	struct simd_mem_f128
	{
		using reg_t = __m128;

		[[nodiscard]] static inline reg_t load_aligned(const float* p) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			return _mm_load_ps(p);
		}

		[[nodiscard]] static inline reg_t load_unaligned(const float* p) noexcept { return _mm_loadu_ps(p); }

		static inline void store_aligned(float* p, reg_t v) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			_mm_store_ps(p, v);
		}

		static inline void store_unaligned(float* p, reg_t v) noexcept { _mm_storeu_ps(p, v); }
	};

	struct simd_mem_d128
	{
		using reg_t = __m128d;

		[[nodiscard]] static inline reg_t load_aligned(const double* p) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			return _mm_load_pd(p);
		}

		[[nodiscard]] static inline reg_t load_unaligned(const double* p) noexcept { return _mm_loadu_pd(p); }

		static inline void store_aligned(double* p, reg_t v) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			_mm_store_pd(p, v);
		}

		static inline void store_unaligned(double* p, reg_t v) noexcept { _mm_storeu_pd(p, v); }
	};

	template <typename T>
		requires std::is_integral_v<T>
	struct simd_mem_i128
	{
		using reg_t = __m128i;

		[[nodiscard]] static inline reg_t load_aligned(const T* p) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			return _mm_load_si128(reinterpret_cast<const reg_t*>(p));
		}

		[[nodiscard]] static inline reg_t load_unaligned(const T* p) noexcept { return _mm_loadu_si128(reinterpret_cast<const reg_t*>(p)); }

		static inline void store_aligned(T* p, reg_t v) noexcept
		{
			DEBUG_ASSERT(reinterpret_cast<uintptr_t>(p) % 16 == 0);
			_mm_store_si128(reinterpret_cast<reg_t*>(p), v);
		}

		static inline void store_unaligned(T* p, reg_t v) noexcept { _mm_storeu_si128(reinterpret_cast<reg_t*>(p), v); }
	};

	struct simd_bitwise_i128
	{
		using reg_t = __m128i;

		static inline reg_t bit_and(reg_t a, reg_t b) noexcept { return _mm_and_si128(a, b); }
		static inline reg_t bit_or(reg_t a, reg_t b) noexcept { return _mm_or_si128(a, b); }
		static inline reg_t bit_xor(reg_t a, reg_t b) noexcept { return _mm_xor_si128(a, b); }
		static inline reg_t bit_not(reg_t a) noexcept { return _mm_xor_si128(a, _mm_set1_epi32(-1)); }
	};

	struct simd_bitwise_f128
	{
		using reg_t = __m128;

		static inline reg_t bit_and(reg_t a, reg_t b) noexcept { return _mm_and_ps(a, b); }
		static inline reg_t bit_or(reg_t a, reg_t b) noexcept { return _mm_or_ps(a, b); }
		static inline reg_t bit_xor(reg_t a, reg_t b) noexcept { return _mm_xor_ps(a, b); }
		static inline reg_t bit_not(reg_t a) noexcept { return _mm_xor_ps(a, _mm_castsi128_ps(_mm_set1_epi32(-1))); }
	};

	struct simd_bitwise_d128
	{
		using reg_t = __m128d;

		static inline reg_t bit_and(reg_t a, reg_t b) noexcept { return _mm_and_pd(a, b); }
		static inline reg_t bit_or(reg_t a, reg_t b) noexcept { return _mm_or_pd(a, b); }
		static inline reg_t bit_xor(reg_t a, reg_t b) noexcept { return _mm_xor_pd(a, b); }
		static inline reg_t bit_not(reg_t a) noexcept { return _mm_xor_pd(a, _mm_castsi128_pd(_mm_set1_epi32(-1))); }
	};

	template <typename T>
	struct simd_dispatch;

	template <>
	struct simd_dispatch<int8_t> : simd_mem_i128<int8_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<int8_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi8(a, b); }
		static inline reg_t add_s(reg_t a, reg_t b) noexcept { return _mm_adds_epi8(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi8(a, b); }
		static inline reg_t sub_s(reg_t a, reg_t b) noexcept { return _mm_subs_epi8(a, b); }
		static inline reg_t broadcast(int8_t v) noexcept { return _mm_set1_epi8(v); }

		static inline uint32_t movemask(reg_t v) noexcept { return static_cast<uint32_t>(_mm_movemask_epi8(v)); }

		static inline reg_t set(int8_t i1, int8_t i2, int8_t i3, int8_t i4, int8_t i5, int8_t i6, int8_t i7, int8_t i8, int8_t i9, int8_t i10,
					int8_t i11, int8_t i12, int8_t i13, int8_t i14, int8_t i15, int8_t i16) noexcept
		{
			return _mm_setr_epi8(i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15, i16);
		}

		static inline reg_t cmpeq(reg_t a, reg_t b) noexcept { return _mm_cmpeq_epi8(a, b); }

		static inline reg_t cmplt(reg_t a, reg_t b) noexcept { return _mm_cmplt_epi8(a, b); }
	};

	template <>
	struct simd_dispatch<uint8_t> : simd_mem_i128<uint8_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<uint8_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi8(a, b); }
		static inline reg_t add_s(reg_t a, reg_t b) noexcept { return _mm_adds_epu8(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi8(a, b); }
		static inline reg_t sub_s(reg_t a, reg_t b) noexcept { return _mm_subs_epu8(a, b); }
		static inline reg_t broadcast(uint8_t v) noexcept { return _mm_set1_epi8(static_cast<int8_t>(v)); }

		static inline uint32_t movemask(reg_t v) noexcept { return static_cast<uint32_t>(_mm_movemask_epi8(v)); }

		static inline reg_t set(uint8_t i1, uint8_t i2, uint8_t i3, uint8_t i4, uint8_t i5, uint8_t i6, uint8_t i7, uint8_t i8, uint8_t i9, uint8_t i10,
					uint8_t i11, uint8_t i12, uint8_t i13, uint8_t i14, uint8_t i15, uint8_t i16) noexcept
		{
			return _mm_set_epi8(static_cast<uint8_t>(i1),
					    static_cast<uint8_t>(i2),
					    static_cast<uint8_t>(i3),
					    static_cast<uint8_t>(i4),
					    static_cast<uint8_t>(i5),
					    static_cast<uint8_t>(i6),
					    static_cast<uint8_t>(i7),
					    static_cast<uint8_t>(i8),
					    static_cast<uint8_t>(i9),
					    static_cast<uint8_t>(i10),
					    static_cast<uint8_t>(i11),
					    static_cast<uint8_t>(i12),
					    static_cast<uint8_t>(i13),
					    static_cast<uint8_t>(i14),
					    static_cast<uint8_t>(i15),
					    static_cast<uint8_t>(i16));
		}

		static inline reg_t cmpeq(reg_t a, reg_t b) noexcept { return _mm_cmpeq_epi8(a, b); }

		static inline reg_t cmplt(reg_t a, reg_t b) noexcept { return _mm_cmplt_epi8(a, b); }
	};

	template <>
	struct simd_dispatch<int16_t> : simd_mem_i128<int16_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<int16_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi16(a, b); }
		static inline reg_t add_s(reg_t a, reg_t b) noexcept { return _mm_adds_epi16(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi16(a, b); }
		static inline reg_t sub_s(reg_t a, reg_t b) noexcept { return _mm_subs_epi16(a, b); }
		static inline reg_t broadcast(int16_t v) noexcept { return _mm_set1_epi16(v); }
		static inline reg_t set(int16_t i1, int16_t i2, int16_t i3, int16_t i4, int16_t i5, int16_t i6, int16_t i7, int16_t i8) noexcept
		{
			return _mm_set_epi16(i1, i2, i3, i4, i5, i6, i7, i8);
		}

		static inline reg_t cmplt(reg_t a, reg_t b) noexcept { return _mm_cmplt_epi16(a, b); }
	};

	template <>
	struct simd_dispatch<uint16_t> : simd_mem_i128<uint16_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<uint16_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi16(a, b); }
		static inline reg_t add_s(reg_t a, reg_t b) noexcept { return _mm_adds_epu16(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi16(a, b); }
		static inline reg_t sub_s(reg_t a, reg_t b) noexcept { return _mm_subs_epu16(a, b); }
		static inline reg_t broadcast(uint16_t v) noexcept { return _mm_set1_epi16(static_cast<int16_t>(v)); }
		static inline reg_t set(uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4, uint16_t i5, uint16_t i6, uint16_t i7, uint16_t i8) noexcept
		{
			return _mm_set_epi16(static_cast<uint16_t>(i1),
					     static_cast<uint16_t>(i2),
					     static_cast<uint16_t>(i3),
					     static_cast<uint16_t>(i4),
					     static_cast<uint16_t>(i5),
					     static_cast<uint16_t>(i6),
					     static_cast<uint16_t>(i7),
					     static_cast<uint16_t>(i8));
		}

		static inline reg_t cmplt(reg_t a, reg_t b) noexcept { return _mm_cmplt_epi16(a, b); }
	};

	template <>
	struct simd_dispatch<int32_t> : simd_mem_i128<int32_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<int32_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi32(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi32(a, b); }
		static inline reg_t mul(reg_t a, reg_t b) noexcept { return _mm_mullo_epi32(a, b); }
		static inline reg_t broadcast(int32_t v) noexcept { return _mm_set1_epi32(v); }
		static inline reg_t set(int32_t i1, int32_t i2, int32_t i3, int32_t i4) noexcept { return _mm_set_epi32(i1, i2, i3, i4); }
	};

	template <>
	struct simd_dispatch<uint32_t> : simd_mem_i128<uint32_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<uint32_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi32(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi32(a, b); }
		static inline reg_t mul(reg_t a, reg_t b) noexcept { return _mm_mullo_epi32(a, b); }
		static inline reg_t broadcast(uint32_t v) noexcept { return _mm_set1_epi32(static_cast<int32_t>(v)); }
		static inline reg_t set(uint32_t i1, uint32_t i2, uint32_t i3, uint32_t i4) noexcept
		{
			return _mm_set_epi32(static_cast<uint32_t>(i1), static_cast<uint32_t>(i2), static_cast<uint32_t>(i3), static_cast<uint32_t>(i4));
		}
	};

	template <>
	struct simd_dispatch<int64_t> : simd_mem_i128<int64_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<int64_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi64(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi64(a, b); }
		static inline reg_t broadcast(int64_t v) noexcept { return _mm_set1_epi64x(v); }
		static inline reg_t set(int64_t i1, int64_t i2) noexcept { return _mm_set_epi64x(i1, i2); }
	};

	template <>
	struct simd_dispatch<uint64_t> : simd_mem_i128<uint64_t>, simd_bitwise_i128
	{
		using reg_t = typename simd_mem_i128<uint64_t>::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_epi64(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_epi64(a, b); }
		static inline reg_t broadcast(uint64_t v) noexcept { return _mm_set1_epi64x(static_cast<int64_t>(v)); }
		static inline reg_t set(uint64_t i1, uint64_t i2) noexcept { return _mm_set_epi64x(static_cast<int64_t>(i1), static_cast<int64_t>(i2)); }
	};

	template <>
	struct simd_dispatch<float> : simd_mem_f128, simd_bitwise_f128
	{
		using reg_t = typename simd_mem_f128::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_ps(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_ps(a, b); }
		static inline reg_t mul(reg_t a, reg_t b) noexcept { return _mm_mul_ps(a, b); }
		static inline reg_t div(reg_t a, reg_t b) noexcept { return _mm_div_ps(a, b); }
		static inline reg_t broadcast(float v) noexcept { return _mm_set1_ps(v); }
		static inline reg_t set(float f1, float f2, float f3, float f4) noexcept { return _mm_set_ps(f1, f2, f3, f4); }
		static inline reg_t unpack_lo(reg_t a, reg_t b) noexcept { return _mm_unpacklo_ps(a, b); }
		static inline reg_t unpack_hi(reg_t a, reg_t b) noexcept { return _mm_unpackhi_ps(a, b); }

		template <int Imm8>
		static inline reg_t shuffle(reg_t a, reg_t b) noexcept
		{
			return _mm_shuffle_ps(a, b, Imm8);
		}
	};

	template <>
	struct simd_dispatch<double> : simd_mem_d128, simd_bitwise_d128
	{
		using reg_t = typename simd_mem_d128::reg_t;

		static inline reg_t add(reg_t a, reg_t b) noexcept { return _mm_add_pd(a, b); }
		static inline reg_t sub(reg_t a, reg_t b) noexcept { return _mm_sub_pd(a, b); }
		static inline reg_t mul(reg_t a, reg_t b) noexcept { return _mm_mul_pd(a, b); }
		static inline reg_t div(reg_t a, reg_t b) noexcept { return _mm_div_pd(a, b); }
		static inline reg_t broadcast(double v) noexcept { return _mm_set1_pd(v); }
		static inline reg_t set(double d1, double d2) noexcept { return _mm_set_pd(d1, d2); }
		static inline reg_t unpack_lo(reg_t a, reg_t b) noexcept { return _mm_unpacklo_pd(a, b); }
		static inline reg_t unpack_hi(reg_t a, reg_t b) noexcept { return _mm_unpackhi_pd(a, b); }

		template <int Imm8>
		static inline reg_t shuffle(reg_t a, reg_t b) noexcept
		{
			return _mm_shuffle_pd(a, b, Imm8);
		}
	};
} // namespace opus3d::foundation::simd::detail