#pragma once

#ifdef _WIN32

#include <cassert>
#include <cstdint>
#include <string>

namespace opus3d
{
	/**
	 * @brief Converts a UTF-16 buffer to UTF-8 without allocation.
	 *
	 * @param utf16Buffer Pointer to UTF-16 input buffer.
	 * @param utf16BufferLength Number of wchar_t elements to convert.
	 *        This function does NOT assume null termination.
	 * @param outBufferUtf8 Destination buffer for UTF-8 output.
	 * @param outBufferUtf8Length Capacity of destination buffer in bytes.
	 *
	 * @return If the return value is greater than outBufferUtf8Length,
	 *         it represents the required buffer size in bytes and no
	 *         output was written. Otherwise, it represents the number
	 *         of bytes written (not null-terminated).
	 */
	size_t utf16_to_utf8(const wchar_t* utf16Buffer, size_t utf16BufferLength, char* outBufferUtf8,
			     size_t outBufferUtf8Length);
} // namespace opus3d

#endif