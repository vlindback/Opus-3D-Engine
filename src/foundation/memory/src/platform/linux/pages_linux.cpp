#ifdef __linux__

#include <foundation/memory/include/pages.hpp>

#include <cerrno>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cstddef>

namespace opus3d::foundation
{
	static Unexpected<ErrorCode> errno_error() { return Unexpected(ErrorCode::create(error_domains::System, static_cast<uint32_t>(errno))); }

	static int to_posix_protection(MemoryAccess access) noexcept
	{
		// Execute is intentionally not supported (and not representable in MemoryAccess).
		const bool r = has_flag(access, MemoryAccess::Read);
		const bool w = has_flag(access, MemoryAccess::Write);

		// Write-only does not exist in POSIX; treat as bug.
		assert(!(w && !r) && "Write-only memory access is not supported");

		if(!r && !w)
		{
			return PROT_NONE;
		}
		if(r && !w)
		{
			return PROT_READ;
		}
		if(r && w)
		{
			return PROT_READ | PROT_WRITE;
		}

		return PROT_NONE;
	}

	static void linux_try_enable_thp(void* address, size_t size) noexcept
	{
#ifdef MADV_HUGEPAGE
		(void)::madvise(address, size, MADV_HUGEPAGE); // best-effort
#else
		(void)address;
		(void)size;
#endif
	}

	size_t get_system_page_size()
	{
		// If the page size is not retrievable no allocators will work
		// and the whole application will crash spectacularly.

		// Cached to avoid more system calls regarding this.
		static size_t cached = [] {
			const long pageSize = ::sysconf(_SC_PAGESIZE);
			assert(pageSize > 0);
			return static_cast<size_t>(pageSize);
		}();
		return cached;
	}

	Result<std::optional<size_t>> get_system_large_page_size()
	{
		// Under the "consumer distro / no privileges" policy, we do not promise a fixed hugepage size.
		// THP is kernel-controlled and can vary; MAP_HUGETLB requires configuration/privileges.
		return std::optional<size_t>{std::nullopt};
	}

	Result<void*> reserve_pages(size_t size, MemoryPageSize pageSize)
	{
		assert(size > 0);
		assert(size % get_system_page_size() == 0);

		void* result = ::mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if(result == MAP_FAILED)
		{
			return errno_error();
		}

		if(pageSize == MemoryPageSize::Large)
		{
			linux_try_enable_thp(result, size);
		}

		return result;
	}

	Result<void> release_pages(void* address, size_t size)
	{
		assert(address != nullptr);
		assert(size > 0);

		if(::munmap(address, size) != 0)
		{
			return errno_error();
		}

		return {};
	}

	Result<void> set_committed_page_access(void* address, size_t size, MemoryAccess access)
	{
		assert(address != nullptr);
		assert(size > 0);
		assert(size % get_system_page_size() == 0);

		// On Linux, this is also effectively "commit" (demand paging).
		if(::mprotect(address, size, to_posix_protection(access)) != 0)
		{
			return errno_error();
		}

		return {};
	}

	Result<void> set_committed_page_noaccess(void* address, size_t size, GuardMode /*mode*/)
	{
		// Linux "guard" is equivalent to PROT_NONE.
		assert(address != nullptr);
		assert(size > 0);
		assert(size % get_system_page_size() == 0);

		if(::mprotect(address, size, PROT_NONE) != 0)
		{
			return errno_error();
		}

		return {};
	}

	Result<void> commit_pages(void* address, size_t size, MemoryAccess access)
	{
		// Linux doesn't have an explicit "commit"; mprotect to a non-NONE protection makes it usable.
		return set_committed_page_access(address, size, access);
	}

	Result<void> decommit_pages(void* address, size_t size)
	{
		assert(address != nullptr);
		assert(size > 0);
		assert(size % get_system_page_size() == 0);

		// Make inaccessible again.
		if(::mprotect(address, size, PROT_NONE) != 0)
		{
			return errno_error();
		}

		// Best-effort reclaim of physical pages.
#ifdef MADV_DONTNEED
		(void)::madvise(address, size, MADV_DONTNEED);
#endif

		return {};
	}

	Result<void*> allocate_pages(size_t size, MemoryAccess access)
	{
		auto reserved = reserve_pages(size, MemoryPageSize::Normal);
		if(!reserved)
			return Unexpected(reserved.error());

		auto committed = commit_pages(reserved.value(), size, access);
		if(!committed)
		{
			// Avoid leaks if commit fails
			(void)release_pages(reserved.value(), size);
			return Unexpected(committed.error());
		}

		return reserved.value();
	}

	Result<void*> map_file(NativeFileHandle openFileHandle, size_t fileSize, MemoryAccess access)
	{
		assert(openFileHandle.valid());
		assert(fileSize > 0);

		// Expect NativeFileHandle to carry a POSIX fd.
		const int fd = static_cast<int>(openFileHandle.handle);

		const int prot = to_posix_protection(access);

		// MAP_PRIVATE is safest default (copy-on-write). If you want shared writes, add a separate API.
		void* view = ::mmap(nullptr, fileSize, prot, MAP_PRIVATE, fd, 0);

		if(view == MAP_FAILED)
		{
			return errno_error();
		}

		return view;
	}

	Result<void> unmap_file(void* address, size_t size) { return release_pages(address, size); }

} // namespace opus3d::foundation

#endif