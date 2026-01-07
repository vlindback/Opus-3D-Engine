#pragma once

#include "simd_config.hpp"

#include "simd_dispatch.hpp"

namespace opus3d::foundation::simd::detail::concepts
{
	// Helper to get the register type from the dispatch trait
	template <typename T>
	using dispatch_reg_t = typename detail::simd_dispatch<T>::reg_t;

	// Check if the dispatch trait has a 'mul' function
	template <typename T>
	concept supports_mul = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		{ detail::simd_dispatch<T>::mul(a, b) } -> std::same_as<dispatch_reg_t<T>>;
	};

	// Check if the dispatch trait has a 'div' function
	template <typename T>
	concept supports_div = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		{ detail::simd_dispatch<T>::div(a, b) } -> std::same_as<dispatch_reg_t<T>>;
	};

	// Check if the dispatch trait has saturating arithmetic (add_s, sub_s)
	template <typename T>
	concept supports_saturation = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		{ detail::simd_dispatch<T>::add_s(a, b) } -> std::same_as<dispatch_reg_t<T>>;
		{ detail::simd_dispatch<T>::sub_s(a, b) } -> std::same_as<dispatch_reg_t<T>>;
	};

	// Check if bitwise NOT is supported (bitwise_not is tricky for float sometimes)
	template <typename T>
	concept supports_bitwise = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		detail::simd_dispatch<T>::bit_and(a, b);
		detail::simd_dispatch<T>::bit_or(a, b);
		detail::simd_dispatch<T>::bit_xor(a, b);
		detail::simd_dispatch<T>::bit_not(a);
	};

	template <typename T, int Imm>
	concept supports_shuffle = requires(dispatch_reg_t<T> a) {
		{ detail::simd_dispatch<T>::shuffle<Imm>(a, a) } -> std::same_as<dispatch_reg_t<T>>;
	};

	template <typename T>
	concept supports_movemask = requires(dispatch_reg_t<T> a) {
		{ simd_dispatch<T>::movemask(a) } -> std::same_as<uint32_t>;
	};

	template <typename T, typename... Args>
	concept supports_set = requires(Args... args) {
		{ detail::simd_dispatch<T>::set(args...) } -> std::same_as<dispatch_reg_t<T>>;
	};

	template <typename T>
	concept supports_unpack = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		detail::simd_dispatch<T>::unpack_lo(a, b);
		detail::simd_dispatch<T>::unpack_hi(a, b);
	};

	template <typename T>
	concept supports_cmpeq = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		{ detail::simd_dispatch<T>::cmpeq(a, b) } -> std::same_as<dispatch_reg_t<T>>;
	};

	template <typename T>
	concept supports_cmplt = requires(dispatch_reg_t<T> a, dispatch_reg_t<T> b) {
		{ detail::simd_dispatch<T>::cmplt(a, b) } -> std::same_as<dispatch_reg_t<T>>;
	};
}