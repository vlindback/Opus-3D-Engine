#pragma once

#include "error_code.hpp"
#include "error_domain.hpp"

namespace opus3d::foundation
{
	// NOTE: there is no unregister for panic sinks
	// PanicSinks need to be alive for the entirety of the application.

	// This is non negotiable.

	struct PanicContext
	{
		void* ptr;
	};

	using PanicSinkFn = void (*)(const PanicContext&, const char* message, const ErrorCode& error, SourceLocation panicLocation) noexcept;

	struct PanicSink
	{
		PanicSinkFn  fn;
		PanicContext ctx;
	};

	[[noreturn]]
	void panic(const char* message, const ErrorCode& error, SourceLocation panicLocation = SourceLocation()) noexcept;

	[[noreturn]]
	void panic(const char* message, SourceLocation location = SourceLocation::current()) noexcept;

	// Panic sinks and their contexts must remain valid until process termination.
	// Do not register sinks that depend on stack objects or destructed subsystems.
	// Sinks are never unregistered.
	void register_panic_sink(PanicSinkFn fn, PanicContext ctx) noexcept;

	void debug_break_if_3attached() noexcept;

} // namespace opus3d::foundation