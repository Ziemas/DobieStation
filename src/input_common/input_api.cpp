#include "input_api.hpp"
#include "common_input.hpp"

void InputManager::initalize()
{
	// create thread here.
#ifdef __linux__
	input = std::make_unique<LinuxInput>();
#elif WIN32
	input = std::make_unique<WinInput>();
#endif
	input->reset();
}

inputEvent InputManager::poll()
{
	return input->poll();
}
