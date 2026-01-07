#include <foundation/core/include/panic.hpp>

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>

#if !defined(NDEBUG)

#define CALL_DEBUGGER()                                                                                                                                        \
	do                                                                                                                                                     \
	{                                                                                                                                                      \
		if(IsDebuggerPresent())                                                                                                                        \
			__debugbreak();                                                                                                                        \
	} while(0)
#else
#define EMPTY_CALL_DEBUGGER
#endif
#endif

#if defined(EMPTY_CALL_DEBUGGER)
#define CALL_DEBUGGER()                                                                                                                                        \
	do                                                                                                                                                     \
	{                                                                                                                                                      \
	} while(0)

#else
#endif

namespace opus3d::foundation
{
	static PanicSink	   g_sinks[8];
	static std::atomic<size_t> g_sinkCount{0};
	static std::atomic<bool>   g_panic_active{false};

	[[noreturn]]
	void panic(const char* message, const ErrorCode& error, SourceLocation panicLocation) noexcept
	{
		if(g_panic_active.exchange(true, std::memory_order_relaxed))
		{
			std::fputs("PANIC: recursive panic detected\n", stderr);
			std::abort();
		}

		// Call sinks to inform that we are in panic.
		for(size_t i = 0; i < g_sinkCount.load(std::memory_order_relaxed); ++i)
		{
			const PanicSink& sink = g_sinks[i];
			sink.fn(sink.ctx, message, error, panicLocation);
		}

		char errorBuffer[512] = {};
		if(error.domain && error.domain->format)
		{
			error.domain->format(error.code, errorBuffer);
		}

		if constexpr(ErrorCode::has_location())
		{
			std::fputs("Panic at:\n  ", stderr);
			std::fputs(panicLocation.file_name(), stderr);
			std::fputs(":", stderr);
			std::fprintf(stderr, "%u", panicLocation.line());
			std::fputs(" (", stderr);
			std::fputs(panicLocation.function_name(), stderr);
			std::fputs(")\n", stderr);

			// Only print error origin if it's meaningfully different.
			if(error.location.line() != 0 &&
			   (error.location.line() != panicLocation.line() || error.location.file_name() != panicLocation.file_name()))
			{
				std::fputs("Error created at:\n  ", stderr);
				std::fputs(error.location.file_name(), stderr);
				std::fputs(":", stderr);
				std::fprintf(stderr, "%u", error.location.line());
				std::fputs(" (", stderr);
				std::fputs(error.location.function_name(), stderr);
				std::fputs(")\n", stderr);
			}
		}
		else
		{
			std::fputs("Error:\n", stderr);

			std::fputs("  Message: ", stderr);
			std::fputs(message, stderr);
			std::fputs("\n", stderr);

			if(error.domain)
			{
				std::fputs("  Domain: ", stderr);
				std::fputs(error.domain->name, stderr);
				std::fputs("\n", stderr);
			}

			std::fputs("  Code: ", stderr);
			std::fprintf(stderr, "%u\n", error.code);

			if(errorBuffer[0] != '\0')
			{
				std::fputs("  Description: ", stderr);
				std::fputs(errorBuffer, stderr);
				std::fputs("\n", stderr);
			}
		}

// Call attached debuggers.
//debug_break_if_attached();
#ifdef _MSVC
		__debugbreak();
#endif

		// Terminate the program.
		std::abort();
	}

	[[noreturn]]
	void panic(const char* message, SourceLocation location) noexcept
	{
		if(g_panic_active.exchange(true, std::memory_order_relaxed))
		{
			std::fputs("PANIC|RECURSIVE\n", stderr);
			std::abort();
		}

		std::fputs("PANIC\n", stderr);
		std::fputs(message, stderr);
		std::fputs("\n", stderr);

		if constexpr(ErrorCode::has_location())
		{
			std::fputs("At:\n  ", stderr);
			std::fputs(location.file_name(), stderr);
			std::fputs(":", stderr);
			std::fprintf(stderr, "%u", location.line());
			std::fputs(" (", stderr);
			std::fputs(location.function_name(), stderr);
			std::fputs(")\n", stderr);
		}

		//debug_break_if_attached();
		std::abort();
	}

	void register_panic_sink(PanicSinkFn fn, PanicContext ctx) noexcept
	{
		if(g_panic_active.load(std::memory_order_relaxed))
		{
			return;
		}

		const size_t index = g_sinkCount.fetch_add(1, std::memory_order_relaxed);
		if(index < std::size(g_sinks))
		{
			g_sinks[index] = PanicSink{.fn = fn, .ctx = ctx};
		}
	}

	void debug_break_if_attached() noexcept { CALL_DEBUGGER(); }

} // namespace opus3d::foundation