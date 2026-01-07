#include <foundation/memory/include/linear_allocator.hpp>

namespace opus3d::foundation::memory
{
	Result<LinearAllocator> LinearAllocator::create(size_t reservedBytes) { return LinearAllocator(reservedBytes); }

	LinearAllocator::LinearAllocator(size_t reserveSize) {}

	void* LinearAllocator::allocate(size_t size, size_t alignment) noexcept { return nullptr; }

	Result<void*> LinearAllocator::try_allocate(size_t size, size_t alignment) noexcept { return nullptr; }

	void LinearAllocator::reset() noexcept {}
} // namespace opus3d::foundation::memory