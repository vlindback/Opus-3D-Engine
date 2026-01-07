#pragma once

#include "error_domain.hpp"

#include <cstdint>
#include <span>

namespace opus3d::foundation::error_domains
{
	/**
	 * @brief Converts a platform-specific system error code into a human-readable UTF-8 string.
	 * * This function retrieves the OS error description (via strerror_r on POSIX or 
	 * FormatMessageW on Windows), trims trailing whitespace/newlines, and ensures 
	 * the result is null-terminated.
	 * * @details **Error Resolution Hierarchy:**
	 * 1. Attempt to retrieve the official OS error message.
	 * 2. If the OS code is unknown, format as a numeric fallback (e.g., "POSIX error 2").
	 * 3. If the buffer is too small for the numeric fallback, copy a static "error format" string.
	 * 4. If the buffer cannot even fit that, the buffer is set to an empty string.
	 * * **Constraints:**
	 * - Guaranteed **no dynamic allocations** (uses internal stack buffers).
	 * - Guaranteed **no exceptions**.
	 * - **Thread-safe**: Uses reentrant OS functions.
	 * * @param code The system error code (e.g., errno on Linux or GetLastError() on Windows).
	 * @param strBuffer The destination string.
	 * * @note If @p bufferSize is 0, the function returns immediately without writing.
	 * @note Output is guaranteed to be UTF-8 encoded.
	 * @note If the message is truncated due to insufficient buffer size,
	 *       the output is still guaranteed to be null-terminated.
	 * @returns The number of bytes written (including null terminator).
	 */
	size_t system_error_formatter(uint32_t code, std::span<char> strBuffer) noexcept;

	// Defined in headers as inline constexpr so every TU sees the same address:

	inline constexpr ErrorDomain System = {"System", system_error_formatter};
} // namespace opus3d::foundation::error_domains