#include <engine/core/include/engine.hpp>

#include <foundation/application/include/event_poller.hpp>
#include <foundation/core/include/assert.hpp>
#include <foundation/window/include/window.hpp>

namespace opus3d::engine
{
	enum class ExitReason
	{
		None,
		UserRequested,
		OSRequested,
		FatalError
	};

	int exit_reason_to_code(ExitReason reason) noexcept { return (reason == ExitReason::FatalError) ? 1 : 0; }

	Engine::Engine(const EngineConfig& config) noexcept : m_context{.config = config} {}

	Engine::~Engine() noexcept {}

	void Engine::attach_primary_window(foundation::Window& window) noexcept
	{
		DEBUG_ASSERT(m_state == State::Constructed);
		m_primaryWindow = &window;
	}

	void Engine::attach_event_poller(foundation::app::IEventPoller& poller) noexcept
	{
		DEBUG_ASSERT(m_state == State::Constructed);
		m_appEventPoller = &poller;
	}

	[[nodiscard]] Result<void> Engine::initialize() noexcept
	{
		if(!state_transition(State::Constructed, State::Initializing))
		{
			//  return Err(EngineError::InvalidStateTransition,
			// "Engine::initialize called in invalid state");
		}

		// --- perform initialization here ---
		// logging, memory, jobs, platform, renderer, etc.

		set_state(State::Initialized);

		return {};
	}

	[[nodiscard]] Result<RunResult> Engine::run() noexcept
	{
		// Verify valid resources.

		DEBUG_ASSERT(m_appEventPoller);
		DEBUG_ASSERT(m_primaryWindow);

		if(!state_transition(State::Initialized, State::Running))
		{
			//return Err(EngineError::InvalidStateTransition, "Engine::run called before initialization or after shutdown");
		}

		Result<RunResult> result = main_loop();

		// Transition to shutdown
		set_state(State::ShuttingDown);

		shutdown_internal();

		set_state(State::Shutdown);

		return result;
	}

	void Engine::shutdown() noexcept {}

	void Engine::request_exit(ExitReason reason) noexcept
	{
		m_exitReason.store(reason, std::memory_order_relaxed);
		m_exitRequested.store(true, std::memory_order_release);
	}

	bool Engine::exit_requested() const noexcept { return m_exitRequested.load(std::memory_order_acquire); }

	const EngineContext& Engine::context() const noexcept { return m_context; }

	bool Engine::state_transition(State expectedState, State newState) noexcept
	{
		return m_state.compare_exchange_strong(expectedState, newState, std::memory_order_acq_rel);
	}

	void Engine::set_state(State newState) noexcept { m_state.store(newState, std::memory_order_release); }

	Result<RunResult> Engine::main_loop() noexcept
	{
		while(!exit_requested())
		{
			// pump events

			if(m_appEventPoller->poll_events(foundation::app::PollEventsMode::Poll))
			{
				// begin frame

				// timing (tick clocks)

				// aquire frame context (ring buffer)

				// gather tasks

				// network input (main thread or job)

				// simulation fixed steps

				// variable update (camera, animation etc)

				// renderer: build render packet / render graph

				// execute tasks

				// main-thread only work + present

				// end frame
			}
			else
			{
				// TODO: how do we know if it was the OS or the User that send the quit message?
				request_exit(ExitReason::UserRequested);
			}
		}

		const ExitReason exitReason = m_exitReason.load();
		const int	 exitCode   = exit_reason_to_code(exitReason);

		return RunResult{.reason = exitReason, .exitCode = exitCode};
	}

	void Engine::shutdown_internal() noexcept {}

} // namespace opus3d::engine