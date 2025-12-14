#include "..\tests\test_framework.hpp"

#include "foundation/fibers/fiber.hpp"

#include <memory>

namespace opus3d::tests
{
	BEGIN_TEST(Foundation, Fibers, BasicSwitchReturn) {

		bool ran = false;

		auto stack = std::make_unique<char[]>(64 * 1024);

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto&) {
			ran = true; // Should execute exactly once
		});

		ASSERT_TRUE(!fiber.done()); // Not done until first resume
		fiber.resume();
		ASSERT_TRUE(ran);
		ASSERT_TRUE(fiber.done());
	}

	BEGIN_TEST(Foundation, Fibers, MultiStepSwitching) {

		int counter = 0;

		auto stack = std::make_unique<char[]>(64 * 1024);

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto& self) {
			counter++;
			self.yield(); // yield first time

			counter++;
			self.yield(); // yield second time

			counter++;
			// return → done
		});

		ASSERT_EQ(counter, 0);

		fiber.resume();
		ASSERT_EQ(counter, 1);

		fiber.resume();
		ASSERT_EQ(counter, 2);

		fiber.resume();
		ASSERT_EQ(counter, 3);
		ASSERT_TRUE(fiber.done());
	}

	BEGIN_TEST(Foundation, Fibers, NestedFibers) {

		int  order  = 0;
		auto stackA = std::make_unique<char[]>(64 * 1024);
		auto stackB = std::make_unique<char[]>(64 * 1024);

		auto lmbd = [&](auto& selfA) {
			order = 1;

			auto fiberB = new opus3d::foundation::Fiber(stackB.get(), 64 * 1024, [&](auto& selfB) {
				order = 2;
				selfB.yield(); // go back to A
				order = 3;
			});

			fiberB->resume(); // into B (order == 2)
			order = 4;
		};

		opus3d::foundation::Fiber<decltype(lmbd)>* fiberB = nullptr;

		opus3d::foundation::Fiber fiberA(stackA.get(), 64 * 1024, lmbd);

		fiberA.resume();
		ASSERT_EQ(order, 1); // After handshake start

		fiberA.resume();
		ASSERT_EQ(order, 2); // Fiber B first run

		fiberA.resume();
		ASSERT_EQ(order, 4); // Returned from B to A

		delete fiberB;
	}

	BEGIN_TEST(Foundation, Fibers, MultipleFibersSequence) {

		std::vector<int> execution;
		auto		 stack1 = std::make_unique<char[]>(64 * 1024);
		auto		 stack2 = std::make_unique<char[]>(64 * 1024);

		opus3d::foundation::Fiber fiber1(stack1.get(), 64 * 1024, [&](auto& self) {
			execution.push_back(1);
			self.yield();
			execution.push_back(1);
		});

		opus3d::foundation::Fiber fiber2(stack2.get(), 64 * 1024, [&](auto& self) {
			execution.push_back(2);
			self.yield();
			execution.push_back(2);
		});

		// Round-robin style
		fiber1.resume(); // 1
		fiber2.resume(); // 1,2
		fiber1.resume(); // 1,2,1
		fiber2.resume(); // 1,2,1,2

		ASSERT_EQ(execution.size(), 4);
		ASSERT_EQ(execution[0], 1);
		ASSERT_EQ(execution[1], 2);
		ASSERT_EQ(execution[2], 1);
		ASSERT_EQ(execution[3], 2);

		ASSERT_TRUE(fiber1.done());
		ASSERT_TRUE(fiber2.done());
	}

} // namespace opus3d::tests