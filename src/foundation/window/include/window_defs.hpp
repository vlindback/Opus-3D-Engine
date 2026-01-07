#pragma once

namespace opus3d::foundation
{
	enum class WindowMode
	{
		Windowed,
		BorderlessFullscreen,
		TrueFullscreen
	};

	struct WindowExtent
	{
		int width;
		int height;
	};

	struct WindowPosition
	{
		int x;
		int y;
	};

	struct WindowCreateDesc
	{
		WindowMode mode = WindowMode::Windowed;

		const char* title = "";

		// Only meaningful when mode == Windowed
		std::optional<WindowExtent> extent;

		// Optional future extensions:
		// int monitorIndex;
		// int refreshRate;
		// bool resizable;
	};

	enum class WindowEventType
	{
		CloseRequested,
		Resize,
		FocusGained,
		FocusLost,
		SurfaceSuspended, // minimized, occluded
		SurfaceResumed,	  // restored
		SurfaceDestroyed, // gone
		DpiChanged
	};

	struct WindowEvent
	{
		WindowEventType type;

		union
		{
			struct
			{
				WindowExtent extent;
			} resized;

			struct
			{
				float dpi_scale;
			} dpi;
		};
	};

} // namespace opus3d::foundation