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

		// Find delimiters
		const size_t delimiterPos1 = input.find('/');
		const size_t delimiterPos2 =
			delimiterPos1 == std::string::npos ? std::string::npos : input.find('/', delimiterPos1 + 1);
		const size_t delimiterPos3 =
			delimiterPos2 == std::string::npos ? std::string::npos : input.find('/', delimiterPos2 + 1);

		// Reject if more than two slashes
		if(delimiterPos3 != std::string::npos) {
			return Result{empty_sv, empty_sv, empty_sv};
		}

		// Only category provided: "Category"
		if(delimiterPos1 == std::string::npos) {
			return Result{std::string_view(input.data(), input.size()), empty_sv, empty_sv};
		}

		// Category and suite: "Category/Suite"
		if(delimiterPos2 == std::string::npos) {
			return Result{std::string_view(input.data(), delimiterPos1),
				      std::string_view(input.data() + delimiterPos1 + 1,
						       input.length() - (delimiterPos1 + 1)),
				      empty_sv};
		}

		// Full path: "Category/Suite/Test"
		return Result{std::string_view(input.data(), delimiterPos1),
			      std::string_view(input.data() + delimiterPos1 + 1, delimiterPos2 - (delimiterPos1 + 1)),
			      std::string_view(input.data() + delimiterPos2 + 1, input.length() - (delimiterPos2 + 1))};
	}

	// Run a single test and print protocol lines
	static bool run_single_test(Test* test) {
		const std::string fullName = test->testCategory + "/" + test->testSuite + "/" + test->testName;

		bool passed = false;

		std::cout << "TEST_START " << fullName << "\n";

		try {
			test->run();
			std::cout << "TEST_PASSED " << fullName << "\n";
			passed = true;
		} catch(const TestFailure& e) {
			std::cout << "TEST_FAILED " << fullName << " " << e.what() << " File: " << e.file << ":"
				  << e.line << "\n";
		} catch(...) {
			std::cout << "TEST_FAILED " << fullName << " unknown_exception\n";
		}

		std::cout << "TEST_END " << fullName << "\n";

		return passed;
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
		int		   failures    = 0;
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
