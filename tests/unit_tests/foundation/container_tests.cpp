#include "../tests/test_framework.hpp"

#include <foundation/containers/include/vector_dynamic.hpp>
#include <foundation/containers/include/vector_static.hpp>

#include <foundation/memory/include/heap_allocator.hpp>

namespace opus3d::tests
{
	// General all purpose allocator
	// These tests exist for container functionality not allocator <=> container interactions.
	foundation::HeapAllocator globalHeapAllocator;

	BEGIN_TEST(Foundation, Containers, VectorStatic)
	{
		using namespace foundation;

		VectorStatic<int, 20> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

		ASSERT_TRUE(numbers.size() == 10);

		// Check that back() fetches properly.
		int v = numbers.back();
		ASSERT(v == 10);

		// Check that pop_back removes correctly.
		numbers.pop_back();
		ASSERT_TRUE(numbers.size() == 9);

		// Check sane find_if
		const int     valueToFind = 5;
		std::optional f		  = numbers.find_if([](int x) { return x == valueToFind; });
		ASSERT_TRUE(f.has_value());
		ASSERT(f.value() > 0 && f.value() < numbers.size());
		ASSERT(numbers[f.value()] == valueToFind);

		// Check clear.
		numbers.clear();

		// TODO: check that VectorStatic calls destructors properly! Use something other then int.
	}

	BEGIN_TEST(Foundation, Containers, VectorDynamic)
	{
		using namespace foundation;

		VectorDynamic<int> numbers(as_allocator(globalHeapAllocator));

		// Take no space in memory by default.
		ASSERT_TRUE(numbers.capacity() == 0);

		const size_t numbersCount = 48;

		// MSVC crashed here, need to use <void> explicitly.
		Result<void> r = numbers.try_reserve(numbersCount);
		// Allocator shouldn't fail (it's malloc but just in case)
		ASSERT_TRUE(r.has_value());

		// No waste, no over-allocation.
		ASSERT_TRUE(numbers.capacity() == numbersCount);

		for(size_t i = 0; i < numbersCount; ++i)
		{
			numbers.push_back(i);
		}

		// Check that the numbers are in right order.
		for(size_t i = 0; i < numbersCount; ++i)
		{
			ASSERT(numbers[i] == i);
		}
	}

} // namespace opus3d::tests