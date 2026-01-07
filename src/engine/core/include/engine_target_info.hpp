#pragma once

#include <cstdint>

namespace opus3d::engine
{
	enum class TargetOS
	{
		Windows,
		Linux,
		Android,
		MacOS,
	};

	enum class PlatformClass
	{
		Desktop,
		Mobile,
		Console,
	};

	enum class Architecture
	{
		x86_64,
		arm64,
	};

	enum class PrecisionMode
	{
		Float32,
		Float64,
	};

	// Build metadata for the engine.
	// PURPOSE: For crash reporting or diagnostics its very useful if the
	// engine can self report what it was built for.
	struct EngineTargetInfo
	{
		/* constexpr static const Version engineVersion; */
		/* constexpr static const Version foundationVersion*/

		constexpr static TargetOS      targetOS	     = TargetOS::Windows;
		constexpr static Architecture  architecture  = Architecture::x86_64;
		constexpr static PlatformClass platformClass = PlatformClass::Desktop;

		constexpr static const char* compilerName    = "MSVC";
		constexpr static uint32_t    compilerVersion = _MSC_FULL_VER;

		constexpr static const char* buildConfiguration = "Release";
		constexpr static const char* engineVersion	= "0.1.0";

		// Math / simulation
		// Controls what level of numerical accuracy the engine guarantee for simulation-relevant data.
		// It controls:
		// - World space representation & Transforms
		// - Physics-adjacent math
		// - Determinism expectations (timestamps)
		// - Serialization formats.
		constexpr static const PrecisionMode precision = PrecisionMode::Float32;
	};
} // namespace opus3d::engine