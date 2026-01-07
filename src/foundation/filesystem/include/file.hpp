#pragma once

#include <foundation/core/include/result.hpp>

#include <foundation/core/include/platform_types.hpp>

#include <cstdint>

#include "path_view8.hpp"

namespace opus3d::foundation::fs
{
	struct FileHandle
	{
		NativeFileHandle handle;

		inline bool valid() const noexcept { return handle != NativeFileHandleInvalid; }
	};

	struct DirHandle
	{
		NativeFileHandle handle;

		inline bool valid() const noexcept { return handle != NativeFileHandleInvalid; }
	};

	enum class FileAccess : uint8_t
	{
		Read,
		Write,
		ReadWrite
	};
	enum class FileMode : uint8_t
	{
		OpenExisting,
		CreateAlways,
		CreateNew,
		OpenOrCreate,
		TruncateExisting
	};
	enum class FileShare : uint8_t
	{
		None,
		Read,
		Write,
		ReadWrite,
		Delete,
		All
	}; // Windows-like; POSIX can approximate
	enum class FileFlags : uint8_t
	{
		None	     = 0,
		Sequential   = 1, // access pattern hint
		Random	     = 2, // access pattern hint
		NoBuffering  = 4, // bypass OS cache, this hard hard requirements on Windows please read the documentation.
		WriteThrough = 8  // durability guarantee
	};

	constexpr FileFlags operator|(FileFlags a, FileFlags b) noexcept { return static_cast<FileFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }

	constexpr bool operator&(FileFlags a, FileFlags b) noexcept { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

	// Files

	Result<FileHandle> file_open(PathView8 path, FileAccess access, FileMode mode, FileShare share = FileShare::ReadWrite,
				     FileFlags flags = FileFlags::None) noexcept;

	Result<void> file_close(FileHandle handle) noexcept;

	// Directories

	Result<DirHandle> dir_open(PathView8 path) noexcept;
	Result<void>	  dir_close(DirHandle) noexcept;

} // namespace opus3d::foundation::fs