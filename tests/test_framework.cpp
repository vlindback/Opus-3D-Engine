#include "test_framework.hpp"

#include <algorithm>
#include <string_view>
#include <tuple>
#include <vector>

namespace opus3d::tests
{

	std::tuple<std::string_view, std::string_view, std::string_view>
	extract_test_components(const std::string& input) {
		using Result = std::tuple<std::string_view, std::string_view, std::string_view>;
		// The empty string_view for rejection cases
		const std::string_view empty_sv{};

		// Find the first delimiter
		size_t delimiterPos1 = input.find('/');

		// Check if the first '/' was found
		if(delimiterPos1 == std::string::npos) {
			// Format rejected: No '/' found (e.g., "Category")
			return Result{empty_sv, empty_sv, empty_sv};
		}

		// Find the second delimiter, starting *after* the first one
		size_t delimiterPos2 = input.find('/', delimiterPos1 + 1);

		// Check if the second '/' was found
		if(delimiterPos2 == std::string::npos) {
			// Format rejected: Only one '/' found (e.g., "Category/Suite")
			return Result{empty_sv, empty_sv, empty_sv};
		}

		// Ensure no third delimiter exists
		size_t delimiterPos3 = input.find('/', delimiterPos2 + 1);

		if(delimiterPos3 != std::string::npos) {
			// Format rejected: Three or more '/' found (e.g., "Cat/Suite/Test/Extra")
			return Result{empty_sv, empty_sv, empty_sv};
		}

		// Format is Test"Category/TestSuite/Test"
		std::string_view testCategory(input.data(), delimiterPos1);
		std::string_view testSuite(input.data() + delimiterPos1 + 1, delimiterPos2 - (delimiterPos1 + 1));
		std::string_view testName(input.data() + delimiterPos2 + 1, input.length() - (delimiterPos2 + 1));

		return Result{testCategory, testSuite, testName};
	}

	// Run a single test and print protocol lines
	static bool run_single_test(Test* test) {
		const std::string full_name = test->testCategory + "/" + test->testSuite + "/" + test->testName;

		std::cout << "TEST_START " << full_name << "\n";

		try {
			test->run();
			std::cout << "TEST_PASS " << full_name << "\n";
			return true;
		} catch(const std::exception& e) {
			std::cout << "TEST_FAIL " << full_name << " " << e.what() << "\n";
			return false;
		} catch(...) {
			std::cout << "TEST_FAIL " << full_name << " unknown_exception\n";
			return false;
		}
	}

	int TestController::execute_all() {
		int failures = 0;

		for(Test* test : tests) {
			if(!run_single_test(test)) {
				++failures;
			}
		}

		return failures;
	} // namespace opus3d::tests

	int TestController::execute_filtered(const std::string& filter) {
		int failures = 0;

		std::cout << "filter: " << filter << '\n';

		std::vector<Test*> sortedTests = tests;

		std::sort(sortedTests.begin(), sortedTests.end(), [](const Test* a, const Test* b) {
			if(a->testCategory != b->testCategory) {
				return a->testCategory < b->testCategory;
			}
			if(a->testSuite != b->testSuite) {
				return a->testSuite < b->testSuite;
			}
			return a->testName < b->testName;
		});

		auto [testCategory, testSuite, testName] = extract_test_components(filter);

		std::cout << "comp: " << testCategory << "/" << testSuite << "/" << testName << '\n';

		for(Test* test : tests) {

			bool execute = false;

			if(test->testCategory == testCategory) {

				if(!testSuite.empty()) {
					if(!testName.empty()) {
						execute =
							(test->testSuite == testSuite) && (test->testName == testName);
					} else {
						execute = test->testSuite == testSuite;
					}
				} else {
					execute = true;
				}
			}

			if(execute) {
				run_single_test(test);
			}
		}

		return failures;
	}

	void TestController::list_tests(std::ostream& os) const {
		for(Test* test : tests) {
			os << "TEST " << test->testCategory << "/" << test->testSuite << "/" << test->testName << "\n";
		}
	}

} // namespace opus3d::tests