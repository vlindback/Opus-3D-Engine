#pragma once

#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
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

	struct TestFailure : std::exception
	{
		const char* file;
		int	    line;
		std::string assertion;
		std::string message;

		TestFailure(const char* file_, int line_, std::string assertion_, std::string message_)
		    : file(file_), line(line_), assertion(std::move(assertion_)), message(std::move(message_)) {}

		const char* what() const noexcept override { return message.c_str(); }
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

#define TEST_FAIL(assertion, message) throw ::opus3d::tests::TestFailure(__FILE__, __LINE__, assertion, message)

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
	do {                                                                                                           \
		if(!(expr)) {                                                                                          \
			TEST_FAIL("ASSERT_TRUE", #expr);                                                               \
		}                                                                                                      \
	} while(0)

#define ASSERT_FALSE(expr)                                                                                             \
	do {                                                                                                           \
		if((expr)) {                                                                                           \
			TEST_FAIL("ASSERT_FALSE", #expr);                                                              \
		}                                                                                                      \
	} while(0)

#define ASSERT_EQ(expected, actual)                                                                                    \
	do {                                                                                                           \
		if((expected) != (actual)) {                                                                           \
			TEST_FAIL("ASSERT_EQ", std::format("Expected {} == {}, but got {} != {}", #expected, #actual,  \
							   (expected), (actual)));                                     \
		}                                                                                                      \
	} while(0)

// Passes if |actual - expected| <= tolerance
#define ASSERT_NEAR(actual, expected, tolerance)                                                                       \
	do {                                                                                                           \
		auto _a = static_cast<long double>(actual);                                                            \
		auto _e = static_cast<long double>(expected);                                                          \
		auto _t = static_cast<long double>(tolerance);                                                         \
                                                                                                                       \
		if(std::isnan(_a) || std::isnan(_e)) {                                                                 \
			TEST_FAIL("ASSERT_NEAR", "NaN encountered");                                                   \
		}                                                                                                      \
                                                                                                                       \
		auto _diff = std::fabsl(_a - _e);                                                                      \
		if(_diff > _t) {                                                                                       \
			TEST_FAIL("ASSERT_NEAR",                                                                       \
				  std::format("Expected |{} - {}| = {} > {}", #actual, #expected, _diff, _t));         \
		}                                                                                                      \
	} while(0)