#include <foundation/application/include/event_poller.hpp>

#include <Windows.h>

namespace opus3d::foundation::app
{
	bool OSEventPoller::poll_events(PollEventsMode mode) const noexcept
	{
		MSG msg;

		if(mode == PollEventsMode::WaitForEvents)
		{
			// Blocking wait.
			// Wait until there is *something* to do:
			// - window/input message
			// - or timeout (or other handles later)
			MsgWaitForMultipleObjectsEx(0,
						    nullptr,
						    INFINITE, // or bounded timeout later
						    QS_ALLINPUT,
						    MWMO_INPUTAVAILABLE);
		}

		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return true;
	}
} // namespace opus3d::foundation::app