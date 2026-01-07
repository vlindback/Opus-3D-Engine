#pragma once

#include "input_event.hpp"
#include "window_defs.hpp"

namespace opus3d::foundation
{
	class IWindowEventListener
	{
	public:

		virtual void on_window_event(WindowEvent event) noexcept = 0;
	};
} // namespace opus3d::foundation