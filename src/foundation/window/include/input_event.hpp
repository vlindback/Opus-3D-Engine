#pragma once

#include <cstdint>

namespace opus3d::foundation
{
	using InputDeviceId = uint32_t;

	enum class MouseButton : uint8_t
	{
		Left,
		Right,
		Middle,
		ExtraButton1,
		ExtraButton2,
		ExtraButton3,
		ExtraButton4,
		ExtraButton5,
		ExtraButton6,
		ExtraButton7,
		ExtraButton8,
		ExtraButton9,
		ExtraButton10,
		ExtraButton11,
		ExtraButton12,
		ExtraButton13,
		LIMIT_MaxButtons = 16
	};

	constexpr const size_t TextChunkByteMax = 8;

	using InputTimestamp = uint64_t; // nanoseconds since engine start

	enum class InputEventType
	{
		KeyboardKey,
		MouseButton,
		MouseMove,
		MouseWheel,
		TextStart,
		TextUpdate,
		TextEnd,
		Touch,
		GamepadAxis,
		GamepadButton,
		Hid,
		PlatformError
	};

	struct InputEvent
	{
		InputTimestamp timeStamp;
		InputEventType type;
		InputDeviceId  device;

		union
		{
			struct
			{
				uint32_t scancode; // physical key
				uint32_t keycode;  // logical key (layout-aware)
				bool	 pressed;
			} keyboard;

			struct
			{
				MouseButton button;
				bool	    pressed;
			} mouse_button;

			struct
			{
				float dx;
				float dy;
				bool  relative;
			} mouse_move;

			struct
			{
				float delta;
			} mouse_wheel;

			struct
			{
				uint8_t	 bytes[TextChunkByteMax]; // enough for a few UTF-8 codepoints
				uint16_t chunkId;
				int16_t	 cursor;
				int16_t	 selection; // optional
				uint8_t	 length;
			} text;

			struct
			{
				uint8_t finger;
				float	x;
				float	y;
				float	pressure;
			} touch;

			struct
			{
				uint8_t axis;
				float	value;
			} gamepad_axis;

			struct
			{
				uint8_t button;
				bool	pressed;
			} gamepad_button;

			struct
			{
				uint16_t usage_page;
				uint16_t usage;
				int32_t	 value;
			} hid;
			struct
			{
				uint32_t platform;  // e.g. Win32, X11, Wayland, Android
				uint32_t subsystem; // Input, Window, IME, RawInput, HID, etc.
				uint32_t code;	    // e.g. GetLastError(), HRESULT, errno
			} error;
		};
	};
} // namespace opus3d::foundation