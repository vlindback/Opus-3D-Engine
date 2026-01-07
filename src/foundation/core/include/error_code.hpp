#pragma once

#include <cstdint>
#include <source_location>

#include "error_domain.hpp"
#include "source_location.hpp"
#include "system_error.hpp"

#include <foundation/core/include/platform_types.hpp>

namespace opus3d::foundation
{
	/**
	 * @brief Represents a domain-scoped error code with optional source location.
	 *
	 * @details
	 * An ErrorCode consists of:
	 * - A pointer to an ErrorDomain describing the error category and formatting rules.
	 * - A domain-specific numeric error value.
	 * - Optional source location information (file, function, line), enabled only in
	 *   debug builds.
	 *
	 * ErrorCode is a lightweight, trivially copyable value type intended to be passed
	 * by value and used as the error payload for Result<T>.
	 *
	 * @note In non-debug builds, source location information occupies no storage due to
	 *       [[no_unique_address]] and is considered logically absent.
	 */
	struct ErrorCode
	{
		/** Pointer to the error domain that owns this error code. */
		const ErrorDomain* domain;

		/** Domain-specific numeric error value. */

		uint32_t code;

		/**
		 * @brief Source location where the error was created.
		 *
		 * @note This field is only meaningful in debug builds.
		 *       In release builds it occupies no storage and contains no diagnostic data.
		 */
		[[no_unique_address]] SourceLocation location;

		/**
		 * @brief Constructs an ErrorCode associated with a specific domain and value.
		 *
		 * @param domain The error domain describing this error category.
		 * @param code   The domain-specific error value.
		 * @param location Optional source location. Defaults to the call site.
		 *
		 * @returns A fully initialized ErrorCode instance.
		 */
		static constexpr ErrorCode create(const ErrorDomain& domain, uint32_t code, SourceLocation location = SourceLocation::current())
		{
			return {&domain, code, location};
		}

		/**
		 * @brief Indicates whether this ErrorCode carries source location information.
		 *
		 * @details
		 * This function returns a compile-time constant indicating whether source
		 * location data is enabled for this build configuration.
		 *
		 * @warning
		 * This function should not be used in a runtime `if` statement to conditionally
		 * access @ref location. Although the return value is constant, the compiler is
		 * not required to eliminate the branch.
		 *
		 * To conditionally include debug-only diagnostic code, use `if constexpr`
		 * with a compile-time trait instead.
		 *
		 * @example
		 * @code
		 * void append_error_message(String& message, const ErrorCode& error)
		 * {
		 *     message.append(format_error(error));
		 *
		 *     // Correct: compile-time removal in release builds
		 *     if constexpr (ErrorCode::has_location())
		 *     {
		 *         message.append(" @ ");
		 *         message.append(error.location.file_name());
		 *         message.append(":");
		 *         message.append(error.location.line());
		 *     }
		 * }
		 * @endcode
		 *
		 * @returns `true` in debug builds, `false` in release builds.
		 */
		constexpr static bool has_location() noexcept
		{
#ifdef FOUNDATION_DEBUG
			return true;
#else
			return false;
#endif
		}
	};

} // namespace opus3d::foundation