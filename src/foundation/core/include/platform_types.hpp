#pragma once

#include <cstdint>

namespace opus3d
{

#ifdef _WIN32

	/* A native file handle (HANDLE) for Windows. */
	using NativeFileHandle = void*;

	constexpr NativeFileHandle NativeFileHandleInvalid = nullptr;

	using NativeWindowHandle = void*;

	using NativeErrorCodeType = uint32_t; // Matches Windows DWORD

#elif defined __linux__

	/* A native file descriptor (int) for Linux. */
	using NativeFileHandle = int;

	constexpr NativeFileHandle NativeFileHandleInvalid = -1;

	using NativeWindowHandle = void*;

	using NativeErrorCodeType = int; // Matches POSIX errno

#endif

} // namespace opus3d