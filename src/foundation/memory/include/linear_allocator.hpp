#pragma once

#include <foundation/core/include/panic.hpp>
#include <foundation/core/include/result.hpp>

#include "allocator.hpp"
#include "virtual_range.hpp"

#include <cstddef>

namespace opus3d::foundation::memory
{
	class LinearAllocator
	{
	public:

		static Result<LinearAllocator> create(size_t reservedBytes);

		explicit LinearAllocator(size_t reserveSize);

		void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept;

		Result<void*> try_allocate(size_t size, size_t alignment) noexcept;

		void reset() noexcept; // O(1)

		size_t marker() const noexcept { return m_offset; }

		void reset_to(size_t m) noexcept
		{
			ASSERT(m <= m_offset);
			m_offset = m;
		}

		size_t used() const noexcept { return m_offset; }
		size_t peak_used() const noexcept { return m_peak; }
		size_t committed() const noexcept { return m_range.size(); }
		size_t capacity() const noexcept { return m_range.capacity(); }

		std::byte*	 base() noexcept { return m_range.data(); }
		const std::byte* base() const noexcept { return m_range.data(); }

	private:

		LinearAllocator() = default;

	private:

		VirtualRange m_range;
		size_t	     m_offset = 0;
		size_t	     m_peak   = 0;
	};

	// Helper functions:

	Allocator as_allocator(LinearAllocator& a) noexcept
	{
		static auto freeNoop	  = [](void*, void*, size_t, size_t) noexcept {};
		static auto linearAllocFn = [](void* ctx, size_t size, size_t alignment) noexcept {
			return static_cast<LinearAllocator*>(ctx)->try_allocate(size, alignment);
		};

		return Allocator(&a, linearAllocFn, freeNoop);
	}

} // namespace opus3d::foundation::memory