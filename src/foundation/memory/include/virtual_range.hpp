#pragma once

#include <foundation/core/include/result.hpp>

#include <memory>

namespace opus3d::foundation::memory
{
	/**
    	 * @brief Ownership of a contiguous virtual address range with a moving "usable" boundary.
    	 * * If hasGuardPage is true, an extra page is silently reserved to act as a 
    	 * moving "fence" immediately following the committed memory.
    	 */
	class VirtualRange
	{
	public:

		VirtualRange() = default;

		// Move-only semantics
		VirtualRange(const VirtualRange&)	     = delete;
		VirtualRange& operator=(const VirtualRange&) = delete;

		VirtualRange(VirtualRange&&) noexcept;
		VirtualRange& operator=(VirtualRange&&) noexcept;

		~VirtualRange();

		// Factory method, creates a virtual range with at least maxSize usable space.
		// Due to memory pages requiring alignment the actual capacity might be larger.
		[[nodiscard]] static Result<VirtualRange> reserve(size_t maxSize) noexcept;

		// Shrink by deltaSize bytes
		// Note: does _not_ do bounds checking.
		// asserts if it tries to grow beyond max_size()
		[[nodiscard]] Result<void> grow(size_t deltaSize) noexcept;

		// Grow by deltaSize bytes
		// Note: does _not_ do bounds checking.
		// asserts if it tries to shrink below 0.
		[[nodiscard]] Result<void> shrink(size_t deltaSize) noexcept;

		// Size in bytes the class has reserved maximum.
		// Useful for answering how much data does the virtual range ocuppy.
		size_t capacity() const noexcept { return m_reservedSize; }

		// Current committed size
		size_t size() const noexcept { return m_logicalSize; }

		std::byte* data() noexcept { return m_base; }

		const std::byte* data() const noexcept { return m_base; }

	private:

		void reset() noexcept;

	private:

		std::byte* m_base	   = nullptr;
		size_t	   m_committedSize = 0;
		size_t	   m_reservedSize  = 0;
		size_t	   m_logicalSize   = 0;
	};

	// Virutal Range + Guard Page
	class VirtualRangeGuarded
	{
	public:

		VirtualRangeGuarded() = default;

		VirtualRangeGuarded(const VirtualRangeGuarded&)		   = delete;
		VirtualRangeGuarded& operator=(const VirtualRangeGuarded&) = delete;

		VirtualRangeGuarded(VirtualRangeGuarded&& other) noexcept { *this = std::move(other); }
		VirtualRangeGuarded& operator=(VirtualRangeGuarded&& other) noexcept;

		~VirtualRangeGuarded();

		[[nodiscard]] static Result<VirtualRangeGuarded> reserve(size_t maxSize) noexcept;

		[[nodiscard]] Result<void> grow(size_t deltaSize) noexcept;

		[[nodiscard]] Result<void> shrink(size_t deltaSize) noexcept;

		size_t size() const noexcept { return m_logicalSize; }
		size_t committed_size() const noexcept { return m_rwCommittedSize; } // RW only
		size_t capacity() const noexcept { return m_usableReservedSize; }

		std::byte* data() noexcept { return m_base; }

		const std::byte* data() const noexcept { return m_base; }

	private:

		void reset() noexcept;

	private:

		std::byte* m_base		= nullptr;
		size_t	   m_usableReservedSize = 0; // excludes guard page
		size_t	   m_rwCommittedSize	= 0; // page-aligned RW committed bytes
		size_t	   m_logicalSize	= 0; // byte-granular
	};

} // namespace opus3d::foundation::memory