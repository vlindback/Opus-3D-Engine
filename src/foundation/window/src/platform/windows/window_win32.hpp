#pragma once

#include <foundation/core/include/platform_types.hpp>
#include <foundation/memory/include/allocator.hpp>
#include <foundation/window/include/window.hpp>

#include <Windows.h>

#include <optional>

namespace opus3d::foundation
{
	class Window::Impl
	{
	public:

		Impl(Window& window, HWND hwnd) noexcept;

		~Impl() noexcept;

		void show() noexcept;

		void hide() noexcept;

		NativeWindowHandle native_handle() const noexcept { return static_cast<NativeWindowHandle>(m_hwnd); }

		void emit_input_event(const InputEvent& event) noexcept;

		void emit_window_event(const WindowEvent& event) noexcept;

		static Result<memory::UniquePtr<Impl>> create(Window& window, memory::Allocator& allocator, const WindowCreateDesc& desc) noexcept;

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam) noexcept;

		static Window::Impl* get_self(HWND hwnd) noexcept;

		WindowExtent current_extent() const noexcept;

		void on_wm_input(WPARAM wp, LPARAM lp) noexcept;

		void on_ime_composition(LPARAM lp) noexcept;

	private:

		void handle_ime_string(HIMC& hIMC, DWORD imeFlag) noexcept;

		HWND	 m_hwnd = nullptr;
		Window&	 m_self;
		int	 m_width	       = 0;
		int	 m_height	       = 0;
		uint16_t m_textChunkId	       = 0;
		bool	 m_inInteractiveResize = false;
		bool	 m_surfaceSuspended    = false;
		bool	 m_rawInputEnabled     = false;
	};

} // namespace opus3d::foundation