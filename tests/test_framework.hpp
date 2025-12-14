#pragma once

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace opus3d::tests
{
	struct Test
	{
		std::string	      testCategory;
		std::string	      testSuite;
		std::string	      testName;
		std::function<void()> testFunction;

		// The function that the framework will call to execute the test logic.
		virtual void run() = 0;
	};

	// --- 2. Central Register & Controller ---
	class TestController
	{
	private:

		std::vector<Test*> tests;
		TestController() = default;

	public:

		static TestController& get() {
			static TestController instance;
			return instance;
		}

		void register_test(Test* test) { tests.push_back(test); }

		const std::vector<Test*>& get_tests() const { return tests; }

		// Run all tests, return number of failures
		int execute_all();

		// Run only tests matching filter "Suite.Test" (exact match for now)
		int execute_filtered(const std::string& filter);

		// List all tests in a machine-readable way
		void list_tests(std::ostream& os) const;
	};
} // namespace opus3d::tests

// Defines a test class and automatically registers it with the Controller.
#define BEGIN_TEST(TestCategory, TestSuite, TestName)                                                                  \
	class Test_##TestCategory##_##TestSuite##_##TestName : public Test                                             \
	{                                                                                                              \
	public:                                                                                                        \
                                                                                                                       \
		Test_##TestCategory##_##TestSuite##_##TestName() {                                                     \
			testCategory = #TestCategory;                                                                  \
			testSuite    = #TestSuite;                                                                     \
			testName     = #TestName;                                                                      \
			::opus3d::tests::TestController::get().register_test(this);                                    \
		}                                                                                                      \
		void run() override;                                                                                   \
	};                                                                                                             \
	static Test_##TestCategory##_##TestSuite##_##TestName                                                          \
		test_##TestCategory##_##TestSuite##_##TestName##_instance;                                             \
	void	Test_##TestCategory##_##TestSuite##_##TestName::run()

#define ASSERT_TRUE(expr)                                                                                              \
	if(!(expr)) {                                                                                                  \
		throw std::runtime_error("Assertion Failed: " #expr);                                                  \
	}

#define ASSERT_FALSE(expr)                                                                                             \
	if(expr) {                                                                                                     \
		throw std::runtime_error("Assertion Failed: " #expr " is true");                                       \
	}

#define ASSERT_NEQ(expected, actual)                                                                                   \
	if((expected) == (actual)) {                                                                                   \
		throw std::runtime_error("Assertion Failed: Values are equal");                                        \
	}

#define ASSERT_EQ(expected, actual)                                                                                    \
	if((expected) != (actual)) {                                                                                   \
		throw std::runtime_error("Assertion Failed: Expected " + std::to_string(expected) + ", but got " +     \
					 std::to_string(actual) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
	}