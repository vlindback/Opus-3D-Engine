#pragma once

#include "error_code.hpp"
#include "expected.hpp"

namespace opus3d::foundation
{
	template <typename T>
	using Result = Expected<T, ErrorCode>;
}