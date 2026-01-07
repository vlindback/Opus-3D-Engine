#ifdef _WIN32

#include <foundation/memory/include/pages.hpp>

#include <Windows.h>

#include <cassert>

namespace opus3d::foundation
{
	static Unexpected<ErrorCode> last_error(const ErrorDomain& domain = error_domains::System)
	{
		return Unexpected(ErrorCode::create(domain, ::GetLastError()));
	}

	static DWORD to_win32_noaccess(GuardMode mode) noexcept
	{
		if(mode == GuardMode::Guard)
		{
			return PAGE_NOACCESS | PAGE_GUARD;
		}
		return PAGE_NOACCESS;
	}

	static DWORD to_win32_page_protection(MemoryAccess access) noexcept
	{
		const bool r = has_flag(access, MemoryAccess::Read);
		const bool w = has_flag(access, MemoryAccess::Write);

		// Windows does not support write-only
		assert(!(w && !r) && "Write-only memory access is not supported");

		if(!r && !w)
		{
			return PAGE_NOACCESS;
		}
		if(r && !w)
		{
			return PAGE_READONLY;
		}
		if(r && w)
		{
			return PAGE_READWRITE;
		}

		return PAGE_NOACCESS;
	}

	static DWORD to_win32_map_access(MemoryAccess access) noexcept
	{
		DWORD flags = 0;

		if(has_flag(access, MemoryAccess::Read))
		{
			flags |= FILE_MAP_READ;
		}
		if(has_flag(access, MemoryAccess::Write))
		{
			flags |= FILE_MAP_WRITE;
		}

		return flags;
	}

	size_t get_system_page_size() noexcept
	{
		// If the page size is not retrievable no allocators will work
		// and the whole application will crash spectacularly.

		// Cached to avoid more system calls regarding this.
		static size_t cached = [] {
			SYSTEM_INFO info{};
			GetSystemInfo(&info);
			assert(info.dwPageSize > 0);
			return static_cast<size_t>(info.dwPageSize);
		}();
		return cached;
	}

	Result<std::optional<size_t>> get_system_large_page_size() noexcept
	{
		const size_t pageSize = GetLargePageMinimum();
		if(pageSize > 0)
		{
			return std::optional<size_t>(pageSize);
		}
		return std::optional<size_t>(std::nullopt);
	}

	Result<void*> reserve_pages(size_t size, MemoryPageSize pageSize) noexcept
	{
		assert(size > 0);
		assert(size % get_system_page_size() == 0);

		const DWORD flags = MEM_RESERVE | ((pageSize == MemoryPageSize::Large) ? MEM_LARGE_PAGES : 0);

		void* result = VirtualAlloc(nullptr, size, flags, PAGE_NOACCESS);

		if(result == nullptr)
		{
			return last_error();
		}

		return result;
	}

	Result<void> commit_pages(void* address, size_t size, MemoryAccess access) noexcept
	{
		assert(address);
		assert(size);
		assert(size % get_system_page_size() == 0);

		if(VirtualAlloc(address, size, MEM_COMMIT, to_win32_page_protection(access)) == nullptr)
		{
			return last_error();
		}

		return {};
	}

	Result<void> decommit_pages(void* address, size_t size) noexcept
	{
		assert(address);
		assert(size);

		if(VirtualFree(address, size, MEM_DECOMMIT) == 0)
		{
			return last_error();
		}

		return {};
	}

	Result<void*> allocate_pages(size_t size, MemoryAccess access) noexcept
	{
		if(Result reserved = reserve_pages(size); reserved.has_value())
		{
			if(Result committed = commit_pages(reserved.value(), size, access); committed.has_value())
			{
				return reserved.value();
			}
			else
			{
				static_cast<void>(release_pages(reserved.value(), size));
				return Unexpected(committed.error());
			}
		}
		else
		{
			return Unexpected(reserved.error());
		}
	}

	Result<void> release_pages(void* address, size_t size) noexcept
	{

		assert(address != nullptr);
		assert(size > 0);

		if(VirtualFree(address, 0, MEM_RELEASE) == 0)
		{
			return last_error();
		}

		return {};
	}

	Result<void> set_committed_page_access(void* address, size_t size, MemoryAccess access) noexcept
	{
		assert(address);
		assert(size);
		assert(size % get_system_page_size() == 0);

		DWORD oldProtection = 0;
		if(!::VirtualProtect(address, size, to_win32_page_protection(access), &oldProtection))
		{
			return last_error();
		}

		return {};
	}

	Result<void> set_committed_page_noaccess(void* address, size_t size, GuardMode mode) noexcept
	{
		assert(address);
		assert(size);
		assert(size % get_system_page_size() == 0);

		DWORD oldProtection = 0;
		if(!::VirtualProtect(address, size, to_win32_noaccess(mode), &oldProtection))
		{
			return last_error();
		}

		return {};
	}

	Result<void*> map_file(NativeFileHandle openFileHandle, size_t fileSize, MemoryAccess access) noexcept
	{
		assert(openFileHandle.valid());
		assert(fileSize > 0);

		HANDLE file = static_cast<HANDLE>(openFileHandle.handle);

		HANDLE mapping = ::CreateFileMappingW(file,
						      nullptr,
						      to_win32_page_protection(access),
						      static_cast<DWORD>(fileSize >> 32),
						      static_cast<DWORD>(fileSize & 0xFFFFFFFF),
						      nullptr);
		if(!mapping)
		{
			return last_error();
		}

		void* view = MapViewOfFile(mapping, to_win32_map_access(access), 0, 0, fileSize);

		const DWORD mapError = view ? ERROR_SUCCESS : ::GetLastError();

		// Mapping handle can be closed immediately; view keeps the mapping alive.
		::CloseHandle(mapping);

		if(!view)
		{
			return Unexpected(ErrorCode::create(error_domains::System, mapError));
		}

		return view;
	}

	Result<void> unmap_file(void* address, size_t /*size*/) noexcept
	{
		assert(address != nullptr);

		if(!::UnmapViewOfFile(address))
		{
			return last_error();
		}

		return {};
	}

} // namespace opus3d::foundation

#endif