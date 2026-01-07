#pragma once

#include "input_event.hpp"
#include "window_defs.hpp"

namespace opus3d::foundation
{
	class IWindowInputListener
	{
	public:

		virtual void on_input_event(InputEvent event) noexcept = 0;
	};
} // namespace opus3d::foundation