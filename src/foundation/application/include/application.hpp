#pragma once

#include <foundation/containers/include/vector_dynamic.hpp>

#include <foundation/application/include/event_poller.hpp>

namespace opus3d::foundation::app
{
	// Represents the owner of the process / thread message queue.
	// This class is a coordinator that routes messages.
	class Application
	{
	public:

		Application() noexcept;

		const IEventPoller* event_poller() const noexcept { return &m_eventPoller; }

	private:

		const OSEventPoller m_eventPoller;
	};

} // namespace opus3d::foundation::app