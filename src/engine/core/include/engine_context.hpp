#pragma once

#include "engine_config.hpp"
#include "engine_target_info.hpp"

namespace opus3d::engine
{
	struct EngineContext
	{
		// runtime policy
		const EngineConfig& config;

		// build-time facts
		const EngineTargetInfo& target;
	};
} // namespace opus3d::engine