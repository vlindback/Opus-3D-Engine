#pragma once

#include <cstdint>
#include <span>

namespace opus3d::foundation
{
	/**
	 * @brief Describes a category of related error codes and how to format them.
	 *
	 * @details
	 * An ErrorDomain defines:
	 * - A human-readable domain name (for diagnostics and logging).
	 * - A formatter function capable of converting domain-specific error codes
	 *   into readable UTF-8 messages.
	 *
	 * ErrorDomain instances are intended to be defined as static or constexpr
	 * objects with static storage duration. ErrorCode stores a pointer to an
	 * ErrorDomain and does not take ownership.
	 *
	 * Typical examples of error domains include:
	 * - Platform/system errors (errno, GetLastError)
	 * - Graphics API errors (Vulkan, DirectX)
	 * - Engine subsystems (IO, Asset loading, Networking)
	 *
	 * @note ErrorDomain contains no state and is safe to share across threads.
	 */
	struct ErrorDomain
	{
		/**
		 * @brief Function signature used to format domain-specific error codes.
		 *
		 * @param code       The domain-specific numeric error code.
		 * @param str        Destination buffer for the formatted message.
		 * @param bufferSize Size of the destination buffer in bytes.
		 *
		 * @returns The number of bytes written to @p buffer, including the null terminator.
		 *
		 * @note Implementations must:
		 * - Always write a null-terminated string if @p bufferSize > 0.
		 * - Never perform dynamic allocations.
		 * - Be thread-safe.
		 */
		using FormatterFn = size_t (*)(uint32_t, std::span<char>);

		/** Human-readable name of the error domain (e.g., "System", "Vulkan"). */
		const char* name;

		/**
		 * @brief Formatter function used to convert error codes to readable messages.
		 *
		 * @note The formatter must follow the contract described by @ref FormatterFn.
		 */
		FormatterFn format;
	};
} // namespace opus3d::foundation