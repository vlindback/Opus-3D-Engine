#include <engine/input/include/input_system.hpp>

#include <foundation/window/include/window.hpp>

namespace opus3d::engine
{
	InputSystem::InputSystem(foundation::Window& window) noexcept : m_window(window) { m_window.add_listener(this); }

	InputSystem::~InputSystem() noexcept { m_window.remove_listener(this); }
}