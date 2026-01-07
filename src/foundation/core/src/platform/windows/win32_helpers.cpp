#include "win32_helpers.hpp"

#ifdef _WIN32

#include <Windows.h>

namespace opus3d
{
	size_t utf16_to_utf8(const wchar_t* utf16Buffer, size_t utf16BufferLength, char* outBufferUtf8,
			     size_t outBufferUtf8Length)
	{
		assert(utf16Buffer != nullptr);

		// Fast path: attempt conversion
		int written = WideCharToMultiByte(CP_UTF8,
						  0,
						  utf16Buffer,
						  static_cast<int>(utf16BufferLength),
						  outBufferUtf8,
						  static_cast<int>(outBufferUtf8Length),
						  nullptr,
						  nullptr);

		if(written > 0)
		{
			// success, return the count.
			return static_cast<size_t>(written);
		}

		// Query required size (no output written)
		int required = WideCharToMultiByte(
			CP_UTF8, 0, utf16Buffer, static_cast<int>(utf16BufferLength), nullptr, 0, nullptr, nullptr);

		return static_cast<size_t>(required);
	}

} // namespace opus3d

#endif