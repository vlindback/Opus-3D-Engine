#pragma once

#include <foundation/core/include/assert.hpp>

#if defined(__x86_64__) || defined(_M_X64)
#define FOUNDATION_SIMD_X64 1
#elif defined(__aarch64__)
#define FOUNDATION_SIMD_ARM64 1
#else
#error "simd128.hpp: Unsupported architecture (need x64 or ARM64)."
#endif

#if FOUNDATION_SIMD_X64
#include <immintrin.h>
#elif FOUNDATION_SIMD_ARM64
#include <arm_neon.h>
#endif

#include <cmath>
#include <concepts>
#include <cstdint>
#include <type_traits>
