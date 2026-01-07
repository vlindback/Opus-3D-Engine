
#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "window_win32.hpp"

#include <foundation/core/include/assert.hpp>
#include <foundation/core/include/system_error.hpp>

#include <Windows.h>
#include <hidusage.h>
#include <mutex>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <chrono>

namespace opus3d::foundation
{
	std::once_flag	      g_windowClassOnce;
	Result<void>	      g_windowClassResult;
	constexpr const char* g_windowClassName = "cwb";

	inline InputTimestamp now_ns() noexcept
	{
		static const auto start = std::chrono::steady_clock::now();
		const auto	  now	= std::chrono::steady_clock::now();

		return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
	}

	static void register_window_class() noexcept
	{
		WNDCLASSEXA wc	 = {};
		wc.hInstance	 = GetModuleHandle(NULL);
		wc.cbSize	 = sizeof(WNDCLASSEXA);
		wc.lpszClassName = g_windowClassName;
		wc.style	 = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

		if(!RegisterClassExA(&wc))
		{
			g_windowClassResult = Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
			return;
		}

		g_windowClassResult = {};
	}

	static void fast_write_stderr(const char* message, size_t length) noexcept
	{
		HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);
		if(hStderr != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			WriteFile(hStderr, message, static_cast<DWORD>(length), &written, NULL);
		}
	}

	static void error_to_stderr(const char* msg, int errorCode) noexcept
	{
		char buffer[128];
		// Use a "n" version of formatting to prevent overflows
		// snprintf is generally stack-based and won't allocate on the heap
		int len = snprintf(buffer, sizeof(buffer), "%s, %d\n", msg, errorCode);

		if(len > 0)
		{
			fast_write_stderr(buffer, (size_t)len);
		}
	}

	static Result<void> ensure_window_class_registered() noexcept
	{
		std::call_once(g_windowClassOnce, register_window_class);
		return g_windowClassResult;
	}

	bool register_raw_input(HWND hwnd) noexcept
	{
		// Register for RawInput devices
		RAWINPUTDEVICE devices[3];

		// Keyboard
		devices[0].usUsagePage = 0x01;				   // HID_USAGE_PAGE_GENERIC
		devices[0].usUsage     = 0x06;				   // HID_USAGE_GENERIC_KEYBOARD
		devices[0].dwFlags     = RIDEV_NOLEGACY | RIDEV_DEVNOTIFY; // Do not generate legacy key messages
		devices[0].hwndTarget  = hwnd;

		// Mouse
		devices[1].usUsagePage = 0x01;				   // HID_USAGE_PAGE_GENERIC
		devices[1].usUsage     = 0x02;				   // HID_USAGE_GENERIC_MOUSE
		devices[1].dwFlags     = RIDEV_NOLEGACY | RIDEV_DEVNOTIFY; // Do not generate legacy mouse messages
		devices[1].hwndTarget  = hwnd;

		// Gamepad
		devices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;    // 0x01
		devices[2].usUsage     = HID_USAGE_GENERIC_GAMEPAD; // 0x04 or HID_USAGE_GENERIC_GAMEPAD (0x05)
		devices[2].dwFlags     = RIDEV_INPUTSINK;	    // RIDEV_INPUTSINK to receive input even when not in focus
		devices[2].hwndTarget  = hwnd;

		// TODO: report error here somehow with GetLastError()!

		return RegisterRawInputDevices(devices, std::size(devices), sizeof(RAWINPUTDEVICE));
	}

	MouseButton wparam_to_mouse_button(WPARAM wParam) noexcept
	{
		switch(wParam)
		{
			case MK_LBUTTON:
			{
				return MouseButton::Left;
			}
			case MK_RBUTTON:
			{
				return MouseButton::Right;
			}
			case MK_MBUTTON:
			{
				return MouseButton::Middle;
			}
			case MK_XBUTTON1:
			{
				return MouseButton::ExtraButton1;
			}
			case MK_XBUTTON2:
			{
				return MouseButton::ExtraButton2;
			}
			default:
			{
				ASSERT_MSG("Legacy Win32 input with unknown code!");
			}
		}
	}

	static DWORD win32_style_from(WindowMode mode) noexcept
	{
		switch(mode)
		{
			case WindowMode::Windowed:
				// Standard resizable window with caption, system menu, etc.
				return WS_OVERLAPPEDWINDOW;

			case WindowMode::BorderlessFullscreen:
				// Borderless, no caption, no resize frame
				return WS_POPUP;

			case WindowMode::TrueFullscreen:
				// Also WS_POPUP — exclusive fullscreen is handled
				// via display mode changes, not window styles
				return WS_POPUP;

			default:
				// Defensive fallback
				return WS_OVERLAPPEDWINDOW;
		}
	}

	Window::Impl::Impl(Window& window, HWND hwnd) noexcept : m_self(window), m_hwnd(hwnd) {}

	Window::Impl::~Impl() noexcept
	{
		if(m_hwnd != NULL)
		{
			DestroyWindow(m_hwnd);
		}
	}

	void Window::Impl::show() noexcept { ShowWindow(m_hwnd, SW_SHOW); }

	void Window::Impl::hide() noexcept { ShowWindow(m_hwnd, SW_HIDE); }

	void Window::Impl::emit_input_event(const InputEvent& event) noexcept { m_self.emit_input_event(event); }

	void Window::Impl::emit_window_event(const WindowEvent& event) noexcept { m_self.emit_window_event(event); }

	Window::Impl* Window::Impl::get_self(HWND hwnd) noexcept
	{
		LONG_PTR ptr = GetWindowLongPtrA(hwnd, GWLP_USERDATA);
		return reinterpret_cast<Window::Impl*>(ptr);
	}

	WindowExtent Window::Impl::current_extent() const noexcept { return WindowExtent{.width = m_width, .height = m_height}; }

	void Window::Impl::on_wm_input(WPARAM wp, LPARAM lp) noexcept {}

	void Window::Impl::on_ime_composition(LPARAM lp) noexcept
	{
		HIMC hIMC = ImmGetContext(m_hwnd);
		if(hIMC)
		{
			// Composition (preedit)
			if(lp & GCS_COMPSTR)
			{
				handle_ime_string(hIMC, GCS_COMPSTR);
			}

			// Final committed text
			if(lp & GCS_RESULTSTR)
			{
				handle_ime_string(hIMC, GCS_RESULTSTR);
			}

			ImmReleaseContext(m_hwnd, hIMC);
		}
	}

	void get_ime_positions(HIMC& hIMC, DWORD imeFlag, int16_t& cursor, int16_t& selection) noexcept
	{
		if(imeFlag == GCS_COMPSTR)
		{
			// Cursor
			LONG cursorPos = ImmGetCompositionStringA(hIMC, GCS_CURSORPOS, NULL, 0);
			if(cursorPos >= 0)
			{
				cursor = static_cast<int16_t>(cursorPos);
			}

			// Selection (target segment)
			constexpr size_t MaxImeChars = 256;
			uint8_t		 attrBuf[MaxImeChars];

			const LONG attrLen = ImmGetCompositionStringA(hIMC, GCS_COMPATTR, NULL, 0);
			if(attrLen > 0)
			{
				const size_t count = std::min<size_t>(attrLen, MaxImeChars);

				ImmGetCompositionStringA(hIMC, GCS_COMPATTR, attrBuf, static_cast<LONG>(count));

				int start = -1;
				int end	  = -1;

				for(size_t i = 0; i < count; ++i)
				{
					if(attrBuf[i] == ATTR_TARGET_CONVERTED || attrBuf[i] == ATTR_TARGET_NOTCONVERTED)
					{
						if(start == -1)
						{
							start = static_cast<int>(i);
						}
						end = static_cast<int>(i) + 1;
					}
				}

				if(start != -1)
				{
					selection = static_cast<int16_t>(end - start);
				}
			}
		}
	}

	bool Window::Impl::acquire_ime_text(HIMC& hIMC, DWORD imeFlag, char*& buffer, size_t& length, bool& allocated) noexcept
	{
		buffer	  = nullptr;
		length	  = 0;
		allocated = false;

		// Query required byte size
		const LONG compLen = ImmGetCompositionStringA(hIMC, imeFlag, NULL, 0);

		if(compLen == IMM_ERROR_GENERAL || compLen == IMM_ERROR_NODATA)
		{
			return false; // Hard error: do nothing
		}

		// Fast path: small stack buffer
		static constexpr size_t StackBufferSize = 512;
		static char		stackBuffer[StackBufferSize];

		buffer		    = stackBuffer;
		size_t bufferLength = StackBufferSize;

		// Fallback: heap if larger than stack buffer
		if(static_cast<size_t>(compLen) > bufferLength)
		{
			buffer = static_cast<char*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, compLen + 1));

			if(!buffer)
			{
				// Allocation failed: catastrophic but must preserve invariants
				InputEvent ev{};
				ev.timeStamp	  = now_ns();
				ev.type		  = InputEventType::TextUpdate;
				ev.text.chunkId	  = m_textChunkId++;
				ev.text.length	  = 0; // Clear composition
				ev.text.cursor	  = -1;
				ev.text.selection = -1;
				emit_input_event(ev);

				return false;
			}

			bufferLength = compLen + 1;
			allocated    = true;
		}

		// Retrieve the actual text
		const LONG res = ImmGetCompositionStringA(hIMC, imeFlag, buffer, bufferLength);

		if(res == IMM_ERROR_GENERAL || res == IMM_ERROR_NODATA)
		{
			if(allocated)
			{
				HeapFree(GetProcessHeap(), 0, buffer);
			}
			return false;
		}

		// Valid even if res == 0 (empty composition)
		length = static_cast<size_t>(res);
		return true;
	}

	void Window::Impl::emit_empty_ime_update(uint16_t groupId, int16_t cursor, int16_t selection) noexcept
	{
		InputEvent ev{};
		ev.timeStamp	  = now_ns();
		ev.type		  = InputEventType::TextUpdate;
		ev.text.chunkId	  = groupId;
		ev.text.length	  = 0; // Clear
		ev.text.cursor	  = cursor;
		ev.text.selection = selection;

		emit_input_event(ev);
	}

	void Window::Impl::emit_chunked_ime_update(const char* buffer, size_t totalBytes, uint16_t groupId, int16_t cursor, int16_t selection) noexcept
	{
		size_t offset	  = 0;
		bool   firstChunk = true;

		while(offset < totalBytes)
		{
			const size_t chunkSize = std::min(static_cast<size_t>(TextChunkByteMax), totalBytes - offset);

			InputEvent ev{};
			ev.type		= InputEventType::TextUpdate;
			ev.timeStamp	= now_ns();
			ev.text.chunkId = groupId;
			ev.text.length	= static_cast<uint8_t>(chunkSize);

			if(firstChunk)
			{
				ev.text.cursor	  = cursor;
				ev.text.selection = selection;
				firstChunk	  = false;
			}
			else
			{
				ev.text.cursor	  = -1;
				ev.text.selection = -1;
			}

			std::memcpy(ev.text.bytes, buffer + offset, chunkSize);
			emit_input_event(ev);

			offset += chunkSize;
		}
	}

	void Window::Impl::handle_ime_string(HIMC& hIMC, DWORD imeFlag) noexcept
	{
		char*  buffer	 = nullptr;
		size_t length	 = 0;
		bool   allocated = false;

		if(!acquire_ime_text(hIMC, imeFlag, buffer, length, allocated))
		{
			return;
		}

		int16_t cursor	  = -1;
		int16_t selection = -1;

		if(imeFlag == GCS_COMPSTR)
		{
			get_ime_positions(hIMC, imeFlag, cursor, selection);
		}

		const uint16_t groupId = m_textChunkId++;

		if(length == 0)
		{
			emit_empty_ime_update(groupId, cursor, selection);
		}
		else
		{
			emit_chunked_ime_update(buffer, length, groupId, cursor, selection);
		}

		if(allocated)
		{
			HeapFree(GetProcessHeap(), 0, buffer);
		}
	}

	LRESULT CALLBACK Window::Impl::WindowProc(HWND hwnd, UINT msg, LPARAM wParam, WPARAM lParam) noexcept
	{
		Window::Impl* self = get_self(hwnd);

		// If we haven't bound the pointer yet, only handle NCCREATE
		if(self || msg == WM_NCCREATE)
		{
			switch(msg)
			{
				case WM_NCCREATE:
				{
					CREATESTRUCT* cs   = reinterpret_cast<CREATESTRUCT*>(lParam);
					Window::Impl* impl = static_cast<Window::Impl*>(cs->lpCreateParams);
					impl->m_hwnd	   = hwnd;
					SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
					return 1;
				}
				case WM_CLOSE:
				{
					self->emit_window_event({.type = WindowEventType::CloseRequested});
					return 0;
				}
				case WM_SYSCOMMAND:
				{
					// Disable the "Alt" key from freezing the engine to highlight the system menu
					if((wParam & 0xFFF0) == SC_KEYMENU)
					{
						return 0;
					}
					break;
				}
				case WM_ERASEBKGND:
				{
					// Prevent GDI white flicker
					return 1;
				}
				case WM_PAINT:
				{
					ValidateRect(hwnd, NULL);
					return 0;
				}
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
				{
					if(!self->m_rawInputEnabled)
					{
						self->emit_input_event({.timeStamp = now_ns(),
									.type	   = InputEventType::KeyboardKey,
									.keyboard  = {
										 .keycode = static_cast<uint32_t>(wParam),
										 .pressed = true,
									 }});
					}
					return 0;
				}
				case WM_KEYUP:
				case WM_SYSKEYUP:
				{
					if(!self->m_rawInputEnabled)
					{
						self->emit_input_event({.timeStamp = now_ns(),
									.type	   = InputEventType::KeyboardKey,
									.keyboard  = {
										 .keycode = static_cast<uint32_t>(wParam),
										 .pressed = false,
									 }});
					}
					return 0;
				}
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
				{
					if(!self->m_rawInputEnabled)
					{
						self->emit_input_event({.timeStamp    = now_ns(),
									.type	      = InputEventType::MouseButton,
									.mouse_button = {.button  = wparam_to_mouse_button(wParam),
											 .pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
												     msg == WM_MBUTTONDOWN)}});
					}
					return 0;
				}
				case WM_MOUSEMOVE:
				{
					if(!self->m_rawInputEnabled)
					{
						const float x = static_cast<float>(GET_X_LPARAM(lParam));
						const float y = static_cast<float>(GET_Y_LPARAM(lParam));

						self->emit_input_event({.timeStamp  = now_ns(),
									.type	    = InputEventType::MouseMove,
									.mouse_move = {.dx = x, .dy = y, .relative = false}});
					}
					return 0;
				}
				case WM_INPUT:
				{
					if(self->m_rawInputEnabled)
					{
						self->on_wm_input(wParam, lParam);
					}
					return 0;
				}
				case WM_CHAR:
				{
					// We mandate the code page to be UTF8
					// So according to MSDN docs  WM_CHAR arrives as UTF8 bytes.
					const uint8_t b = static_cast<uint8_t>(wParam);
					self->emit_input_event(
						{.timeStamp = now_ns(), .type = InputEventType::TextUpdate, .text = {.bytes = {b}, .length = 1}});
					return 0;
				}
				case WM_IME_COMPOSITION:
				{
					self->on_ime_composition(lParam);
					return 0;
				}
				case WM_IME_STARTCOMPOSITION:
				{
					self->emit_input_event({.timeStamp = now_ns(), .type = InputEventType::TextStart});
					return 0;
				}
				case WM_IME_ENDCOMPOSITION:
				{
					self->emit_input_event({.timeStamp = now_ns(), .type = InputEventType::TextEnd});
					return 0;
				}
				case WM_GETMINMAXINFO:
				{
					// Prevent the window from being resized to 0x0
					MINMAXINFO* mmi	      = reinterpret_cast<MINMAXINFO*>(lParam);
					mmi->ptMinTrackSize.x = 128; // Sensible minimum
					mmi->ptMinTrackSize.y = 128;
					return 0;
				}
				case WM_ENTERSIZEMOVE:
				{
					self->m_inInteractiveResize = true;
					return 0;
				}
				case WM_EXITSIZEMOVE:
				{
					// The user let go! This is the most stable time to recreate the swapchain.
					self->m_inInteractiveResize = false;
					self->emit_window_event({.type = WindowEventType::SurfaceResumed, .resized = self->current_extent()});
					return 0;
				}
				case WM_SIZE:
				{
					self->m_width  = static_cast<int>(LOWORD(lParam));
					self->m_height = static_cast<int>(HIWORD(lParam));

					if(wParam == SIZE_MINIMIZED)
					{
						self->m_surfaceSuspended = true;
						self->emit_window_event({.type = WindowEventType::SurfaceSuspended});
						return 0;
					}

					if(wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
					{
						if(self->m_surfaceSuspended)
						{
							// If we just came back from Taskbar
							self->m_surfaceSuspended = false;
							self->emit_window_event({.type = WindowEventType::SurfaceResumed, .resized = self->current_extent()});
						}
						else if(!self->m_inInteractiveResize)
						{
							// If we are NOT dragging (e.g. Maximize button clicked or Win+Arrow snap)
							self->emit_window_event({.type = WindowEventType::SurfaceResumed, .resized = self->current_extent()});
						}
						else
						{
							// If we ARE dragging, just send a notification (don't recreate swapchain yet)
							self->emit_window_event({.type = WindowEventType::Resize, .resized = self->current_extent()});
						}
					}
					return 0;
				}
				case WM_SETFOCUS:
				{
					self->emit_window_event({.type = WindowEventType::FocusGained});
					return 0;
				}
				case WM_KILLFOCUS:
				{
					self->emit_window_event({.type = WindowEventType::FocusLost});
					return 0;
				}
				case WM_DPICHANGED:
				{
					// Windows defines that 96 DPI = 100 % scaling
					float dpiScale = HIWORD(wParam) / 96.0f;

					self->emit_window_event({.type = WindowEventType::DpiChanged, .dpi = {dpiScale}});

					// Windows suggests a new window rect:
					RECT* suggested = reinterpret_cast<RECT*>(lParam);
					SetWindowPos(hwnd,
						     nullptr,
						     suggested->left,
						     suggested->top,
						     suggested->right - suggested->left,
						     suggested->bottom - suggested->top,
						     SWP_NOZORDER | SWP_NOACTIVATE);

					return 0;
				}
				case WM_DESTROY:
				{
					const WindowEvent event{.type = WindowEventType::SurfaceDestroyed};
					self->emit_window_event(event);
					SetWindowLongPtrA(hwnd, GWLP_USERDATA, NULL);
					PostQuitMessage(0);
					return 0;
				}
			}
		}

		return DefWindowProcA(hwnd, msg, wParam, lParam);
	}

	Result<memory::UniquePtr<Window::Impl>> Window::Impl::create(Window& window, memory::Allocator& allocator, const WindowCreateDesc& desc) noexcept
	{
		if(Result regist = ensure_window_class_registered(); !regist.has_value())
		{
			return Unexpected(regist.error());
		}

		if(Result<memory::UniquePtr<Window::Impl>> alloc = memory::try_make_unique<Window::Impl>(allocator, window); alloc.has_value())
		{
			Impl* impl = alloc.value().get();

			DWORD style   = win32_style_from(desc.mode);
			DWORD exStyle = WS_EX_APPWINDOW;

			// ignore extent
			WindowExtent extentValue = desc.extent.value_or(WindowExtent{.width = CW_USEDEFAULT, .height = CW_USEDEFAULT});

			HWND hwnd = CreateWindowExA(exStyle,
						    g_windowClassName,
						    desc.title,
						    style,
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    extentValue.width,
						    extentValue.height,
						    NULL,
						    NULL,
						    GetModuleHandle(NULL),
						    &impl);

			if(hwnd != NULL)
			{
				// Attempt to register for raw input.

				impl->m_rawInputEnabled = register_raw_input(hwnd);
				if(!impl->m_rawInputEnabled)
				{
					// Let the user know that raw input failed.
					// It's up to the logging system to catch this now.
					error_to_stderr("RawInput failure: ", GetLastError());
				}

				return std::move(alloc.value());
			}
			else
			{
				return Unexpected(ErrorCode::create(error_domains::System, GetLastError()));
			}
		}
		else
		{
			return Unexpected(alloc.error());
		}
	}

} // namespace opus3d::foundation

#endif