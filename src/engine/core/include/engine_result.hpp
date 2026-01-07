#pragma once

#include <foundation/core/include/result.hpp>

namespace opus3d::engine
{
	template <typename T>
	using Result = foundation::Result<T>;

	template <typename T, typename E>
	using Expected = foundation::Expected<T, E>;
} // namespace opus3d::engine