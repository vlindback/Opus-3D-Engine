#include <foundation/core/include/system_error.hpp>

#ifdef __linux__

#include <cerrno>
#include <cstdio>
#include <string.h>

namespace opus3d::foundation
{
	static size_t trim_trailing_whitespace(char* buffer, size_t length)
	{
		while(length > 0)
		{
			char c = buffer[length - 1];
			if(c == '\r' || c == '\n' || c == ' ')
			{
				buffer[--length] = '\0';
			}
			else
			{
				break;
			}
		}
		return length;
	}

	size_t system_formatter(uint32_t code, std::span<char> strBuffer)
	{
		const size_t strBufferSize = strBuffer.size();

		if(strBufferSize == 0)
		{
			return 0;
		}

		buffer[0] = '\0';

		char	    temp[512];
		const char* msg = nullptr;

#if defined(__GLIBC__) && defined(_GNU_SOURCE)
		msg = strerror_r(static_cast<int>(code), temp, sizeof(temp));
#else
		if(strerror_r(static_cast<int>(code), temp, sizeof(temp)) == 0)
		{
			msg = temp;
		}
#endif

		// Primary path: strerror message
		if(msg && msg[0] != '\0')
		{
			int written = std::snprintf(strBuffer.data(), strBufferSize, "%s", msg);

			if(written >= 0)
			{
				size_t used = static_cast<size_t>(written);

				if(used >= strBufferSize)
				{
					// Truncated, but still return a valid C string
					strBuffer[strBufferSize - 1] = '\0';
					used			     = strBufferSize - 1;
				}

				used = trim_trailing_whitespace(strBuffer, used);
				return used + 1; // include null terminator
			}
		}

		// Numeric fallback
		int written = std::snprintf(strBuffer, strBufferSize, "POSIX error %u", code);

		if(written >= 0)
		{
			size_t used = static_cast<size_t>(written);

			if(used >= strBufferSize)
			{
				strBuffer[strBufferSize - 1] = '\0';
				return strBufferSize;
			}

			return used + 1;
		}

		// Hard fallback
		constexpr char	 fallback[]  = "error format";
		constexpr size_t fallbackLen = sizeof(fallback); // includes '\0'

		if(fallbackLen <= strBufferSize)
		{
			std::memcpy(strBuffer.data(), fallback, fallbackLen);
			return fallbackLen;
		}

		strBuffer[0] = '\0';
		return 1;
	}
} // namespace opus3d::foundation

#elif defined(_WIN32)

#include <foundation/core/src/platform/windows/win32_helpers.hpp>

#include <Windows.h>

namespace opus3d::foundation
{
	size_t system_formatter(uint32_t code, std::span<char> strBuffer) noexcept
	{
		const size_t strBufferSize = strBuffer.size();

		if(strBufferSize == 0)
		{
			return 0;
		}

		// Ensure it's always a valid string from the start
		size_t writtenBytes = 1;
		strBuffer[0]	    = '\0';

		wchar_t wideMessage[512];
		DWORD	formatMessageLength = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							     nullptr,
							     code,
							     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							     wideMessage,
							     static_cast<DWORD>(std::size(wideMessage)),
							     nullptr);

		// Trim trailing CR/LF and spaces.
		while(formatMessageLength > 0 && (wideMessage[formatMessageLength - 1] == L'\r' || wideMessage[formatMessageLength - 1] == L'\n' ||
						  wideMessage[formatMessageLength - 1] == L' '))
		{
			--formatMessageLength;
		}

		if(formatMessageLength > 0)
		{
			// Convert UTF-16 -> UTF-8
			writtenBytes = utf16_to_utf8(wideMessage, formatMessageLength, strBuffer.data(), strBufferSize);

			if(writtenBytes > 0)
			{
				if(writtenBytes >= strBuffer.size())
				{
					strBuffer[strBufferSize - 1] = '\0';
					return strBufferSize; // truncated but valid
				}

				strBuffer[writtenBytes] = '\0';
				return writtenBytes + 1;
			}
		}

		// Fallback if OS doesn't know the code or conversion failed
		int writtenFallback = std::snprintf(strBuffer.data(), strBufferSize, "Windows Error 0x%08X", code);

		if(writtenFallback < 0 || static_cast<size_t>(writtenFallback) >= strBufferSize)
		{
			// Two things here.
			// Either we can fit in a static error string.
			// Or we just return an empty string and that in itself says something to us.

			constexpr const char defaultError[] = "error format";

			// Includes null terminator.
			constexpr size_t defaultErrorLength = std::size(defaultError);

			if(defaultErrorLength > strBufferSize)
			{
				// We have done all that we can.
				strBuffer[0] = '\0';
				writtenBytes = 1;
			}
			else
			{
				// The default error message fits.
				std::memcpy(strBuffer.data(), defaultError, defaultErrorLength);
				writtenBytes = defaultErrorLength;
			}
		}
		else
		{
			// snprintf does not include the null terminatorqq.
			writtenBytes = static_cast<size_t>(writtenFallback + 1);
		}

		return writtenBytes;
	}
} // namespace opus3d::foundation

#endif