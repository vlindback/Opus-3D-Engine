#pragma once

namespace opus3d::foundation::app
{
	enum class PollEventsMode
	{
		Poll,	      // Non-blocking
		WaitForEvents // Bounded wait (implementation-defined)
	};

	class IEventPoller
	{
	public:

		virtual bool poll_events(PollEventsMode mode = PollEventsMode::Poll) const noexcept = 0;
	};

	class OSEventPoller : public IEventPoller
	{
	public:

		virtual bool poll_events(PollEventsMode mode = PollEventsMode::Poll) const noexcept override;
	};

} // namespace opus3d::foundation::app