#include "AppController.h"

#include <stdexcept>
#include <iostream>
#include <cstdlib>

int main()
{
	try {
		Vulkan3DEngine::AppController controller{};
		controller.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}