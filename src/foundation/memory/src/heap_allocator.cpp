#include <foundation/core/include/assert.hpp>
#include <foundation/memory/include/heap_allocator.hpp>
#include <foundation/memory/include/memory_error.hpp>

#ifdef _WIN32
#include <malloc.h>
#define OpusMallocAliged _aligned_malloc
#define OpusMallocFree _aligned_free
#else
#include <stdlib.h>

// Wrapper to match the (size, alignment) signature of Windows
inline void* opus_posix_aligned_alloc(size_t size, size_t alignment)
{
	void* ptr = nullptr;
	// POSIX requires alignment to be a power of 2 and a multiple of sizeof(void*)
	if(posix_memalign(&ptr, alignment, size) != 0)
	{
		return nullptr;
	}
	return ptr;
}

#define OpusMallocAligned(size, alignment) opus_posix_aligned_alloc(size, alignment)
#define OpusMallocFree(ptr) free(ptr)

#endif

namespace opus3d::foundation::memory
{
	ErrorCode create_memory_error(MemoryErrorCode errorCode) noexcept { return ErrorCode::create(error_domains::Memory, static_cast<uint32_t>(errorCode)); }

	Result<void*> HeapAllocator::try_allocate(size_t size, size_t alignment) noexcept
	{
		if(size == 0)
		{
			return nullptr;
		}

		// POSIX (but not Windows) requires:
		// alignment >= sizeof(void*)
		// We enforce for consistency:
		alignment = std::max(alignment, alignof(void*));

		// Enforce sane + power of two alignment.
		DEBUG_ASSERT(alignment != 0);
		DEBUG_ASSERT((alignment & (alignment - 1)) == 0);

		if(void* ptr = OpusMallocAliged(size, alignment); ptr)
		{
			m_allocationCount++;
			m_totalAllocatedSize += size;
			return ptr;
		}
		else
		{
			return Unexpected(create_memory_error(MemoryErrorCode::OutOfMemory));
		}
	}

	void HeapAllocator::deallocate(void* ptr, size_t size, size_t) noexcept
	{
		// On POSIX nullptr is fine, on Windows it's undefined behavior.
		// So for consistency:
		if(!ptr)
		{
			return;
		}

		OpusMallocFree(ptr);
		m_totalAllocatedSize -= size;
		m_allocationCount--;
	}

} // namespace opus3d::foundation::memory