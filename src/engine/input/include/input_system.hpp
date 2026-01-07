#pragma once

#include <foundation/window/include/window_input_listener.hpp>

namespace opus3d::foundation
{
	// Forward declarations
	class Window;
} // namespace opus3d::foundation

namespace opus3d::engine
{
	class InputSystem final : public foundation::IWindowInputListener
	{
	public:

		explicit InputSystem(foundation::Window& window) noexcept;
		~InputSystem() noexcept;

		void on_input_event(foundation::InputEvent event) noexcept override;
		void on_window_event(foundation::WindowEvent event) noexcept override;

		void begin_frame() noexcept;
		void end_frame() noexcept;

	private:

		foundation::Window& m_window;
	};
} // namespace opus3d::engine