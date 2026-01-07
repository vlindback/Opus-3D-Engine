#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace opus3d::foundation
{
	template <typename H, typename K>
	concept HashFor = requires(H hasher, K key) {
		// 1. Can we call hasher(key)?
		// 2. Is the result convertible to size_t?
		{ hasher(key) } -> std::convertible_to<std::size_t>;
	} && std::copy_constructible<H>; // Hashers usually need to be copyable for containers
} // namespace opus3d::foundation