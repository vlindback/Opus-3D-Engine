#include <engine/input/include/input_window_listener.hpp>

#include <foundation/window/include/window.hpp>

namespace opus3d::engine
{
	InputWindowListener::InputWindowListener(foundation::memory::Allocator allocator, foundation::Window& window) noexcept :
		m_inputEvents(allocator), m_window(window)
	{
		window.add_window_input_listener(this);
	}

	InputWindowListener::~InputWindowListener() noexcept { m_window.remove_window_input_listener(this); }

	void InputWindowListener::on_input_event(foundation::InputEvent event) noexcept { m_inputEvents.push_back(event); }

	std::span<const foundation::InputEvent> InputWindowListener::get_input_events() const noexcept
	{
		return std::span<const foundation::InputEvent>(m_inputEvents.data(), m_inputEvents.size());
	}

	void InputWindowListener::clear_events() noexcept { m_inputEvents.clear(); }
} // namespace opus3d::engine