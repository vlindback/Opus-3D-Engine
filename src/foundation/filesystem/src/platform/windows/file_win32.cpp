#include <foundation/filesystem/include/file.hpp>

#include <foundation/core/include/system_error.hpp>

#include <Windows.h>

namespace opus3d::foundation::fs
{
	DWORD to_win32(FileAccess access) noexcept
	{
		switch(access)
		{
			case FileAccess::Read:
			{
				return GENERIC_READ;
			}
			case FileAccess::Write:
			{
				return GENERIC_WRITE;
			}
			case FileAccess::ReadWrite:
			{
				return GENERIC_READ | GENERIC_WRITE;
			}
		}
	}

	DWORD to_win32(FileMode mode) noexcept
	{
		switch(mode)
		{
			case FileMode::OpenExisting:
			{
				return OPEN_EXISTING;
			}
			case FileMode::CreateAlways:
			{
				return CREATE_ALWAYS;
			}
			case FileMode::CreateNew:
			{
				return CREATE_NEW;
			}
			case FileMode::OpenOrCreate:
			{
				return OPEN_ALWAYS;
			}
			case FileMode::TruncateExisting:
			{
				return TRUNCATE_EXISTING;
			}
		}
	}

	DWORD to_win32(FileShare share) noexcept
	{
		switch(share)
		{
			case FileShare::Read:
			{
				return FILE_SHARE_READ;
			}
			case FileShare::Write:
			{
				return FILE_SHARE_WRITE;
			}
			case FileShare::ReadWrite:
			{
				return FILE_SHARE_READ | FILE_SHARE_WRITE;
			}
			case FileShare::Delete:
			{
				return FILE_SHARE_DELETE;
			}
			case FileShare::All:
			{
				return FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			}
			default:
			{
				// FileShare::None
				return 0;
			}
		}
	}

	DWORD to_win32(FileFlags flags) noexcept
	{
		DWORD result = FILE_ATTRIBUTE_NORMAL;

		if(flags & FileFlags::Sequential)
		{
			result |= FILE_FLAG_SEQUENTIAL_SCAN;
		}

		if(flags & FileFlags::Random)
		{
			result |= FILE_FLAG_RANDOM_ACCESS;
		}

		if(flags & FileFlags::NoBuffering)
		{
			result |= FILE_FLAG_NO_BUFFERING;
		}

		if(flags & FileFlags::WriteThrough)
		{
			result |= FILE_FLAG_WRITE_THROUGH;
		}

		return result;
	}

	Result<void> close_handle(NativeFileHandle h) noexcept
	{
		if(CloseHandle(h) == 0)
		{
			return Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
		}
		else
		{
			return {};
		}
	}

	Result<FileHandle> file_open(PathView8 path, FileAccess access, FileMode mode, FileShare share, FileFlags flags) noexcept
	{
		// Sequential and random access not compatible.
		DEBUG_ASSERT(!(flags & FileFlags::Sequential && flags & FileFlags::Random));

		DWORD desired_access = to_win32(access);
		DWORD creation	     = to_win32(mode);
		DWORD share_mode     = to_win32(share);
		DWORD file_flags     = to_win32(flags);

		HANDLE h = ::CreateFileA(reinterpret_cast<const char*>(path.data()),
					 desired_access,
					 share_mode,
					 nullptr, // security attributes
					 creation,
					 file_flags,
					 nullptr // template file
		);

		if(h == INVALID_HANDLE_VALUE)
		{
			return Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
		}

		FileHandle fh{};
		fh.handle = h;
		return fh;
	}

	Result<void> file_close(FileHandle handle) noexcept { return close_handle(handle.handle); }

	Result<DirHandle> dir_open(PathView8 path) noexcept
	{
		HANDLE h = CreateFileA(path.c_str(),
				       FILE_LIST_DIRECTORY,
				       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				       nullptr,
				       OPEN_EXISTING,
				       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
				       nullptr);

		if(h == INVALID_HANDLE_VALUE)
		{
			return Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
		}

		FILE_BASIC_INFO info{};
		if(!::GetFileInformationByHandleEx(h, FileBasicInfo, &info, sizeof(info)))
		{
			::CloseHandle(h);
			return Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
		}

		if((info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			::CloseHandle(h);
			return Unexpected(ErrorCode::create(error_domains::System, ERROR_DIRECTORY));
		}

		DirHandle dh;
		dh.handle = h;
		return dh;
	}

	Result<void> dir_close(DirHandle handle) noexcept { return close_handle(handle.handle); }

} // namespace opus3d::foundation::fs