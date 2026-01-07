#include <foundation/memory/include/virtual_range.hpp>

#include <foundation/memory/include/alignment.hpp>
#include <foundation/memory/include/pages.hpp>

#include <foundation/core/include/assert.hpp>

#include <algorithm>

namespace opus3d::foundation
{
	VirtualRange::VirtualRange(VirtualRange&& other) noexcept :
		m_base(other.m_base), m_committedSize(other.m_committedSize), m_logicalSize(other.m_logicalSize), m_reservedSize(other.m_reservedSize)
	{
		other.m_base	      = nullptr;
		other.m_committedSize = 0;
		other.m_logicalSize   = 0;
		other.m_reservedSize  = 0;
	}

	VirtualRange::~VirtualRange() noexcept { reset(); }

	VirtualRange& VirtualRange::operator=(VirtualRange&& other) noexcept
	{
		if(this != &other)
		{
			reset();

			m_base		= std::exchange(other.m_base, nullptr);
			m_committedSize = std::exchange(other.m_committedSize, 0);
			m_logicalSize	= std::exchange(other.m_logicalSize, 0);
			m_reservedSize	= std::exchange(other.m_reservedSize, 0);
		}

		return *this;
	}

	Result<VirtualRange> VirtualRange::reserve(size_t maxSize) noexcept
	{
		// Align up to the closest page system boundary value.
		const size_t pageAlignedMaxSize = align_up(maxSize, get_system_page_size());

		if(Result reserve = reserve_pages(pageAlignedMaxSize, MemoryPageSize::Normal); reserve.has_value())
		{
			VirtualRange range;
			range.m_base	     = static_cast<std::byte*>(reserve.value());
			range.m_reservedSize = pageAlignedMaxSize;
			return range;
		}
		else
		{
			return Unexpected(reserve.error());
		}
	}

	Result<void> VirtualRange::grow(size_t deltaSize) noexcept
	{
		ASSERT_MSG(m_base, "Growing on unreserved VirtualRange!");

		const size_t newLogicalSize = m_logicalSize + deltaSize;

		// Assert that we do not try to grow beyond capacity.
		ASSERT(deltaSize <= m_reservedSize - m_logicalSize);

		const size_t pageSize	      = get_system_page_size();
		const size_t oldCommittedSize = m_committedSize;
		const size_t newCommittedSize = align_up(newLogicalSize, pageSize);

		if(newCommittedSize > oldCommittedSize)
		{
			void*	     commitAddr	   = m_base + oldCommittedSize;
			const size_t bytesToCommit = newCommittedSize - oldCommittedSize;

			if(Result<void> commit = commit_pages(commitAddr, bytesToCommit, MemoryAccess::ReadWrite); !commit.has_value())
			{
				// Return any OS error.
				return commit;
			}
		}

		m_committedSize = newCommittedSize;
		m_logicalSize	= newLogicalSize;

		// Success.
		return {};
	}

	Result<void> VirtualRange::shrink(size_t deltaSize) noexcept
	{
		ASSERT_MSG(m_base, "Shrinking an unreserved VirtualRange!");

		// Assert that we do not try to shrink beyond zero pages.
		ASSERT(deltaSize <= m_logicalSize);

		const size_t pageSize	      = get_system_page_size();
		const size_t newLogicalSize   = m_logicalSize - deltaSize;
		const size_t oldCommittedSize = m_committedSize;
		const size_t newCommittedSize = align_up(newLogicalSize, pageSize);

		if(newCommittedSize < oldCommittedSize)
		{
			const size_t bytesToDecommit = oldCommittedSize - newCommittedSize;
			void*	     decommitAddr    = m_base + newCommittedSize;

			if(Result<void> decommit = decommit_pages(decommitAddr, bytesToDecommit); !decommit.has_value())
			{
				return decommit;
			}
		}

		m_committedSize = newCommittedSize;
		m_logicalSize	= newLogicalSize;

		return {};
	}

	void VirtualRange::reset() noexcept
	{
		if(m_base)
		{
			if(Result<void> release = release_pages(m_base, m_reservedSize); !release.has_value())
			{
				panic("VirtualRange::reset", release.error());
			}

			m_base		= nullptr;
			m_committedSize = 0;
			m_logicalSize	= 0;
			m_reservedSize	= 0;
		}
	}

	VirtualRangeGuarded::~VirtualRangeGuarded() noexcept { reset(); }

	VirtualRangeGuarded& VirtualRangeGuarded::operator=(VirtualRangeGuarded&& other) noexcept
	{
		if(this != &other)
		{
			reset();

			m_base		     = std::exchange(other.m_base, nullptr);
			m_usableReservedSize = std::exchange(other.m_usableReservedSize, 0);
			m_rwCommittedSize    = std::exchange(other.m_rwCommittedSize, 0);
			m_logicalSize	     = std::exchange(other.m_logicalSize, 0);
		}

		return *this;
	}

	Result<VirtualRangeGuarded> VirtualRangeGuarded::reserve(size_t maxSize) noexcept
	{
		const size_t pageSize		= get_system_page_size();
		const size_t usableSize		= align_up(maxSize, pageSize);
		const size_t totalAllocatedSize = usableSize + pageSize; // + 1 guard page.

		if(Result<void*> reserve = reserve_pages(totalAllocatedSize, MemoryPageSize::Normal); !reserve.has_value())
		{
			return Unexpected(reserve.error());
		}
		else
		{
			VirtualRangeGuarded range;
			range.m_base		   = static_cast<std::byte*>(reserve.value());
			range.m_usableReservedSize = usableSize;
			range.m_rwCommittedSize	   = 0;
			range.m_logicalSize	   = 0;

			// Commit + protect the initial guard  page at base.
			// Since rwCommittedSizer==0
			{

				Result<void> result;

				if(result = commit_pages(range.m_base, pageSize, MemoryAccess::ReadWrite); result.has_value())
				{
					if(result = make_guard_pages(range.m_base, pageSize); result.has_value())
					{
						return range;
					}
				}

				// Error, try to clean up the page and return the error.
				range.reset();

				return Unexpected(result.error());
			}
		}
	}

	Result<void> VirtualRangeGuarded::grow(size_t deltaSize) noexcept
	{
		ASSERT_MSG(m_base, "Growing on unreserved VirtualRangeGuarded!");

		const size_t newLogicalSize = m_logicalSize + deltaSize;
		ASSERT(deltaSize <= m_usableReservedSize - m_logicalSize);

		const size_t oldRwCommitted = m_rwCommittedSize;
		const size_t pageSize	    = get_system_page_size();
		const size_t newRwCommitted = align_up(newLogicalSize, pageSize);

		if(newRwCommitted > oldRwCommitted)
		{
			std::byte* oldGuard = m_base + oldRwCommitted;
			std::byte* newGuard = m_base + newRwCommitted;

			// Step A: Commit new capacity (including space for the future guard)
			// Range: [oldGuard + page, newGuard + page)
			std::byte* commitAddr = oldGuard + pageSize;
			size_t	   commitSize = (newRwCommitted - oldRwCommitted);

			if(Result<void> commit = commit_pages(commitAddr, commitSize, MemoryAccess::ReadWrite); !commit.has_value())
			{
				return commit;
			}

			// Step B: Place the new guard
			if(Result<void> guard = make_guard_pages(newGuard, pageSize); !guard.has_value())
			{
				return guard;
			}

			// Step C: Open up the old guard for RW
			if(Result<void> access = set_committed_page_access(oldGuard, pageSize, MemoryAccess::ReadWrite); !access.has_value())
			{
				return access;
			}

			// Finalize state
			m_rwCommittedSize = newRwCommitted;
		}

		m_logicalSize = newLogicalSize;
		return {};
	}

	Result<void> VirtualRangeGuarded::shrink(size_t deltaSize) noexcept
	{
		ASSERT_MSG(m_base, "Shrinking on unreserved VirtualRangeGuarded!");
		ASSERT(deltaSize <= m_logicalSize);

		const size_t pageSize	    = get_system_page_size();
		const size_t newLogical	    = m_logicalSize - deltaSize;
		const size_t oldRwCommitted = m_rwCommittedSize;
		const size_t newRwCommitted = align_up(newLogical, pageSize);

		if(newRwCommitted < oldRwCommitted)
		{
			std::byte* newGuard = m_base + newRwCommitted;

			// Step A: Protect the new guard page.
			if(Result<void> guard = make_guard_pages(newGuard, pageSize); !guard.has_value())
			{
				return guard;
			}

			// Step B: Decommit everything after the new guard page.
			const size_t bytesToDecommit = oldRwCommitted - newRwCommitted;
			void*	     decommitAddr    = newGuard + pageSize;
			if(Result<void> decommit = decommit_pages(decommitAddr, bytesToDecommit); !decommit.has_value())
			{
				return decommit;
			}

			m_rwCommittedSize = newRwCommitted;
		}

		m_logicalSize = newLogical;
		return {};
	}

	void VirtualRangeGuarded::reset() noexcept
	{
		if(m_base)
		{
			const size_t pageSize  = get_system_page_size();
			const size_t totalSize = m_usableReservedSize + pageSize;

			if(Result<void> release = release_pages(m_base, totalSize); !release.has_value())
			{
				panic("VirtualRangeGuarded::reset", release.error());
			}

			m_base		     = nullptr;
			m_usableReservedSize = 0;
			m_rwCommittedSize    = 0;
			m_logicalSize	     = 0;
		}
	}

} // namespace opus3d::foundation