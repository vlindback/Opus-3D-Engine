#pragma once

#include <cstdint>

namespace opus3d::engine
{
	// Policy-level decisions that affect many engine subsystems and cannot be changed casually.
	// If something:
	// - only affects one system -> not EngineConfig
	// - can be toggled per feature -> not EngineConfig
	// - is purely performance tuning -> maybe.
	struct EngineConfig
	{
		// Frame model, Max 4, Min 1
		uint32_t framesInFlight = 2;

		// Memory
		size_t perFrameArenaSizeBytes = 16 * 1024 * 1024;

		// Threading
		uint32_t workerThreadCount = 0; // 0 = auto (system decides)

		// Diagnostics
		bool enableValidation = true;

		// bool enableGpuMarkers = true;
	};
} // namespace opus3d::engine