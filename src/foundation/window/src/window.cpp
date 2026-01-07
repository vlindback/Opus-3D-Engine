#include <foundation/window/include/window.hpp>

#ifdef _WIN32
#include <foundation/window/src/platform/windows/window_win32.hpp>
#endif

namespace opus3d::foundation
{
	Window::Window(memory::Allocator alloc, Application& app) noexcept : m_allocator(alloc), m_windowEventListeners(alloc), m_inputEventListeners(alloc)
	{
		// TODO: register with app?
	}

	Window::Window(Window&& rhs) noexcept :
		m_allocator(rhs.m_allocator), m_windowEventListeners(std::move(rhs.m_windowEventListeners)),
		m_inputEventListeners(std::move(rhs.m_inputEventListeners))
	{}

	Window& Window::operator=(Window&& rhs) noexcept
	{
		if(this != &rhs)
		{
			// TODO IMPLEMENT
		}
		return *this;
	}

	void Window::emit_window_event(const WindowEvent& event) noexcept { notify_window_event_listeners(event); }

	void Window::emit_input_event(const InputEvent& event) noexcept { notify_window_input_listeners(event); }

	void Window::add_window_event_listener(IWindowEventListener* listener) noexcept { m_windowEventListeners.push_back(listener); }

	void Window::remove_window_event_listener(IWindowEventListener* listener) noexcept
	{
		if(std::optional<size_t> index = m_windowEventListeners.find(listener); index)
		{
			m_windowEventListeners.erase_unordered(index.value());
		}
	}

	void Window::add_window_input_listener(IWindowInputListener* listener) noexcept { m_inputEventListeners.push_back(listener); }

	void Window::remove_window_input_listener(IWindowInputListener* listener) noexcept
	{
		if(std::optional<size_t> index = m_inputEventListeners.find(listener); index)
		{
			m_inputEventListeners.erase_unordered(index.value());
		}
	}

	void Window::show() noexcept { m_impl->show(); }

	void Window::hide() noexcept { m_impl->hide(); }

	Result<Window> Window::create(memory::Allocator alloc, Application& app, const WindowCreateDesc& desc) noexcept
	{
		Window window(alloc, app);

		auto impl = Impl::create(window, alloc, desc);
		if(!impl.has_value())
		{
			return Unexpected(impl.error());
		}

		window.m_impl = std::move(impl.value());
		return window;
	}

	NativeWindowHandle Window::handle() noexcept { return m_impl->native_handle(); }

	void Window::notify_window_event_listeners(const WindowEvent& event) noexcept
	{
		for(IWindowEventListener* listener : m_windowEventListeners)
		{
			listener->on_window_event(event);
		}
	}

	void Window::notify_window_input_listeners(const InputEvent& event) noexcept
	{
		for(IWindowInputListener* listener : m_inputEventListeners)
		{
			listener->on_input_event(event);
		}
	}

} // namespace opus3d::foundation