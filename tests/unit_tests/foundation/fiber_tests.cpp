#include "..\tests\test_framework.hpp"

#include "foundation/fibers/fiber.hpp"

#include <memory>

namespace opus3d::tests
{
	// Verifies the most basic fiber lifecycle:
	// a fiber does not run until resumed, runs exactly once,
	// and correctly transitions to the "done" state afterward.
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

	// Verifies that a fiber can yield multiple times and resume execution
	// at the correct instruction point each time.
	// This tests correct instruction pointer and stack preservation.
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

	// Verifies that fibers can safely create and resume other fibers
	// and that nested context switches return control to the correct parent.
	// This is critical for schedulers and task systems.
	BEGIN_TEST(Foundation, Fibers, NestedFibers) {

		int  order  = 0;
		auto stackA = std::make_unique<char[]>(64 * 1024);
		auto stackB = std::make_unique<char[]>(64 * 1024);

		auto fiberA_task = [&](auto& selfA) {
			// Step 1: we are in A
			order = 1;
			selfA.yield(); // let the test observe order == 1

			// Create B on A's stack (IMPORTANT: persists across yields of A)
			auto fiberB_task = [&](auto& selfB) {
				order = 2;
				selfB.yield(); // yield back to A
				order = 3;
				// return -> B done
			};

			opus3d::foundation::Fiber fiberB(stackB.get(), 64 * 1024, fiberB_task);

			// Step 2: run B until it yields (order becomes 2)
			fiberB.resume();
			selfA.yield(); // let the test observe order == 2

			// Step 3: finish B (order becomes 3, then B returns/done)
			fiberB.resume();

			// Step 4: back in A after B completes
			order = 4;
			// return -> A done
		};

		opus3d::foundation::Fiber fiberA(stackA.get(), 64 * 1024, fiberA_task);

		fiberA.resume();
		ASSERT_EQ(order, 1);

		fiberA.resume();
		ASSERT_EQ(order, 2);

		fiberA.resume();
		ASSERT_EQ(order, 4);
		ASSERT_TRUE(fiberA.done());
	}

	// Verifies that multiple independent fibers can be interleaved
	// in a deterministic order without interfering with each other's state.
	// This validates stack isolation between fibers.
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

	// Verifies that local variables stored on a fiber's stack
	// retain their values across yields.
	// This catches stack corruption and misaligned stack pointers.
	BEGIN_TEST(Foundation, Fibers, StackLocalPersistence) {

		auto stack    = std::make_unique<char[]>(64 * 1024);
		int  observed = 0;

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto& self) {
			int local = 123;
			self.yield();
			observed = local;
		});

		fiber.resume();
		fiber.resume();

		ASSERT_EQ(observed, 123);
	}

	// Verifies that large stack frames are handled correctly.
	// This helps detect incorrect stack sizing, guard issues,
	// or ABI-specific stack alignment problems.
	BEGIN_TEST(Foundation, Fibers, LargeStackFrame) {

		auto stack     = std::make_unique<char[]>(128 * 1024);
		bool completed = false;

		opus3d::foundation::Fiber fiber(stack.get(), 128 * 1024, [&](auto& self) {
			char big[32 * 1024];
			memset(big, 0xAB, sizeof(big));
			self.yield();
			completed = true;
		});

		fiber.resume();
		fiber.resume();

		ASSERT_TRUE(completed);
	}

	// Verifies that floating-point registers are preserved
	// across context switches.
	// This is one of the most common failure points when porting
	// fibers across compilers, ABIs, or operating systems.
	BEGIN_TEST(Foundation, Fibers, FloatingPointPreservation) {

		auto   stack  = std::make_unique<char[]>(64 * 1024);
		double result = 0.0;

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto& self) {
			double x = 3.141592653589793;
			self.yield();
			result = x;
		});

		fiber.resume();
		fiber.resume();

		ASSERT_NEAR(result, 3.141592653589793, 1e-12);
	}

	// Verifies that calling resume() on a completed fiber is safe
	// and does not re-run the task or corrupt state.
	BEGIN_TEST(Foundation, Fibers, ResumeAfterDoneIsSafe) {

		auto stack   = std::make_unique<char[]>(64 * 1024);
		int  counter = 0;

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto&) { counter++; });

		fiber.resume();
		ASSERT_TRUE(fiber.done());

		fiber.resume(); // should do nothing
		ASSERT_EQ(counter, 1);
	}

	// Verifies that multiple fibers sharing captured variables
	// do not interfere with each other.
	// This ensures closure state and stack memory remain isolated.
	BEGIN_TEST(Foundation, Fibers, SharedCaptureIsolation) {

		auto stack1 = std::make_unique<char[]>(64 * 1024);
		auto stack2 = std::make_unique<char[]>(64 * 1024);

		int a = 0, b = 0;

		opus3d::foundation::Fiber f1(stack1.get(), 64 * 1024, [&](auto& self) {
			a++;
			self.yield();
			a++;
		});

		opus3d::foundation::Fiber f2(stack2.get(), 64 * 1024, [&](auto& self) {
			b++;
			self.yield();
			b++;
		});

		f1.resume();
		f2.resume();
		f1.resume();
		f2.resume();

		ASSERT_EQ(a, 2);
		ASSERT_EQ(b, 2);
	}

	// Stress test that performs a large number of context switches.
	// This helps catch rare register corruption, stack drift,
	// or incorrect save/restore behavior under heavy switching.
	BEGIN_TEST(Foundation, Fibers, StressSwitching) {

		auto stack = std::make_unique<char[]>(64 * 1024);
		int  count = 0;

		opus3d::foundation::Fiber fiber(stack.get(), 64 * 1024, [&](auto& self) {
			for(int i = 0; i < 10000; ++i) {
				count++;
				self.yield();
			}
		});

		for(int i = 0; i < 10000; ++i) {
			fiber.resume();
		}

		ASSERT_EQ(count, 10000);
	}

} // namespace opus3d::tests