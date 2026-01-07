#pragma once

#include <cstddef>
#include <expected>
#include <optional>

#include <foundation/core/include/result.hpp>

namespace opus3d::foundation::memory
{
	enum class GuardMode : uint8_t
	{
		None, // just inaccessible
		Guard // Windows: PAGE_GUARD|PAGE_NOACCESS, Linux: same as None
	};

	enum class MemoryAccess : uint32_t
	{
		None	  = 0,
		Read	  = 1 << 0,
		Write	  = 1 << 1,
		ReadWrite = Read | Write
		// NOTE: execute is NOT allowed here.
	};

	constexpr MemoryAccess operator|(MemoryAccess a, MemoryAccess b) noexcept
	{
		return static_cast<MemoryAccess>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	constexpr bool has_flag(MemoryAccess value, MemoryAccess flag) noexcept { return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0; }

	enum class MemoryPageSize
	{
		Normal, // Default OS page size
		Large	// @note On Linux, large page requests are best-effort and use madvise(MADV_HUGEPAGE).
	};

	size_t get_system_page_size() noexcept;

	Result<std::optional<size_t>> get_system_large_page_size() noexcept;

	Result<void*> reserve_pages(size_t size, MemoryPageSize pageSize = MemoryPageSize::Normal) noexcept;

	Result<void> commit_pages(void* address, size_t size, MemoryAccess access) noexcept;

	Result<void> decommit_pages(void* address, size_t size) noexcept;

	Result<void*> allocate_pages(size_t size, MemoryAccess access) noexcept;

	Result<void> release_pages(void* address, size_t size) noexcept;

	Result<void> set_committed_page_access(void* address, size_t size, MemoryAccess access) noexcept;

	Result<void> set_committed_page_noaccess(void* address, size_t size, GuardMode mode = GuardMode::None) noexcept;

	Result<void*> map_file(NativeFileHandle openFileHandle, size_t fileSize, MemoryAccess access) noexcept;

	Result<void> unmap_file(void* address, size_t size) noexcept;

	inline Result<void> make_guard_pages(void* address, size_t size) noexcept { return set_committed_page_noaccess(address, size, GuardMode::Guard); }
	inline Result<void> make_noaccess_pages(void* address, size_t size) noexcept { return set_committed_page_noaccess(address, size, GuardMode::None); }

} // namespace opus3d::foundation