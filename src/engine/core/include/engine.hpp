#pragma once

#include "engine_context.hpp"
#include "engine_result.hpp"

#include <atomic>

namespace opus3d::engine
{
	enum class ExitReason
	{
		None,
		UserRequested,
		OSRequested,
		FatalError
	};

	struct RunResult
	{
		ExitReason reason;
		int	   exitCode; // 0 default
	};

	class Engine
	{
	public:

		// Heres the idea:
		//
		//		Engine constructor is not allowed to fail. Period.
		//		That means no syscalls, no memory allocation NOTHING.
		//		It sets up invariants, and gives out system references and nothing more.
		//

		// Phase 0: infallible
		Engine(const EngineConfig& config) noexcept;

		~Engine() noexcept;

		// Phase 1: fallible init
		[[nodiscard]] Result<void> initialize() noexcept;

		// Phase 2: blocking main loop (returns exit code/reason)
		[[nodiscard]] Result<RunResult> run() noexcept;

		void shutdown() noexcept;

		void request_exit(ExitReason reason) noexcept;

		bool exit_requested() const noexcept;

		const EngineContext& context() const noexcept;

	private:

		enum class State : uint8_t
		{
			Constructed,
			Initialized,
			Running,
			ShuttingDown,
			Shutdown
		};

	private:

		// Changes state atomically to newState if the current state matches expectedState.
		bool state_transition(State expectedState, State newState) noexcept;

		// Sets state atomically to newState.
		void set_state(State newState) noexcept;

		Result<RunResult> main_loop() noexcept;

		void shutdown_internal() noexcept;

		const EngineContext	m_context;
		std::atomic<State>	m_state{State::Constructed};
		std::atomic<bool>	m_exitRequested{false};
		std::atomic<ExitReason> m_exitReason{ExitReason::None};
	};
} // namespace opus3d::engine