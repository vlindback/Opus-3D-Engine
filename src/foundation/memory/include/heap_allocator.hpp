#pragma once

#include <foundation/core/include/assert.hpp>
#include <foundation/core/include/result.hpp>

#include "allocator.hpp"

#include <memory>

namespace opus3d::foundation::memory
{
	class HeapAllocator
	{
	public:

		Result<void*> try_allocate(size_t size, size_t alignment) noexcept;

		void* allocate(size_t size, size_t alignment) noexcept
		{
			// MSVC crashed here so we have to use <void*>
			Result<void*> alloc = try_allocate(size, alignment);
			ASSERT_MSG(alloc.has_value(), "Out of memory!");
			return alloc.value();
		}

		void deallocate(void* ptr, size_t size, size_t alignment) noexcept;

		size_t bytes_allocated() const noexcept { return m_totalAllocatedSize; }

		size_t allocation_count() const noexcept { return m_allocationCount; }

	private:

		size_t m_totalAllocatedSize = 0;
		size_t m_allocationCount    = 0;
	};

	// Helper functions:

	inline Allocator as_allocator(HeapAllocator& a) noexcept
	{
		static auto deallocFn = [](void* ctx, void* ptr, size_t size, size_t alignment) noexcept {
			static_cast<HeapAllocator*>(ctx)->deallocate(ptr, size, alignment);
		};
		static auto linearAllocFn = [](void* ctx, size_t size, size_t alignment) noexcept {
			return static_cast<HeapAllocator*>(ctx)->try_allocate(size, alignment);
		};

		return Allocator(&a, linearAllocFn, deallocFn);
	}

} // namespace opus3d::foundation