#pragma once

#include <foundation/containers/include/vector_dynamic.hpp>

#include "window_defs.hpp"
#include "window_event_listener.hpp"
#include "window_input_listener.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace opus3d::foundation
{
	class Application;

	class Window
	{
	public:

		// Factory: returns fully formed Window or error (no exceptions).
		static Result<Window> create(memory::Allocator alloc, Application& app, const WindowCreateDesc& desc) noexcept;

		Window(Window&&) noexcept;
		Window& operator=(Window&&) noexcept;

		Window(const Window&)		 = delete;
		Window& operator=(const Window&) = delete;

		// Events

		void emit_event(const WindowEvent& event) noexcept;
		void emit_event(const InputEvent& event) noexcept;

		// Event listeners
		void add_listener(IWindowListener* listener) noexcept;
		void remove_listener(IWindowListener* listener) noexcept;

		// Visibility & lifecycle
		void show() noexcept;
		void hide() noexcept;

		// Title & geometry
		void set_title(std::string_view title) noexcept;

		WindowMode   mode() const noexcept;
		Result<void> set_mode(WindowMode mode) noexcept; // may fail for TrueFullscreen

		WindowExtent extent() const noexcept;		   // logical window size (points)
		Result<void> resize(WindowExtent extent) noexcept; // windowed mode only (others may ignore or return error)

		std::optional<WindowPosition> position() const noexcept;
		Result<void>		      set_position(WindowPosition pos) noexcept;

		NativeWindowHandle handle() noexcept;

	private:

		class Impl;

	private:

		Window(memory::Allocator alloc, Application& app) noexcept;

		void notify_listeners(const WindowEvent& event) noexcept;
		void notify_listeners(const InputEvent& event) noexcept;

	private:

		memory::Allocator		m_allocator;
		memory::UniquePtr<Impl>		m_impl;
		VectorDynamic<IWindowListener*> m_listeners;
	};
} // namespace opus3d::foundation