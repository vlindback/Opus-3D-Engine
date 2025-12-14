#include "test_framework.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using opus3d::tests::TestController;

int main(int argc, char** argv) {

	auto& controller = TestController::get();

	if(argc >= 2) {
		std::string cmd = argv[1];

		if(cmd == "--list") {
			controller.list_tests(std::cout);
			return 0;
		} else if(cmd == "--run" && argc >= 3) {
			std::string filter   = argv[2]; // e.g. "Foundation/Fibers/TestA"
			int	    failures = controller.execute_filtered(filter);
			return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
		} else {
			std::cerr << "Unknown command. Usage:\n"
				     "  opus3d_tests            # run all tests\n"
				     "  opus3d_tests --list     # list tests\n"
				     "  opus3d_tests --run Category/Suite/Test\n";
			return 1;
		}
	}

	// Default: run all tests
	int failures = controller.execute_all();
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}