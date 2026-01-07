#pragma once

#include <cassert>
#include <type_traits>

namespace opus3d::foundation
{
	/**
	 * @brief Wrapper type used to construct an Expected in the error state.
	 *
	 * @details
	 * Unexpected is a lightweight helper used to explicitly indicate failure
	 * when constructing an Expected<T, E>. It wraps an error value and signals
	 * that the Expected should hold an error instead of a value.
	 *
	 * @example
	 * @code
	 * return Unexpected{ErrorCode::create(error_domains::System, errno)};
	 * @endcode
	 */
	template <typename ErrorT>
	struct Unexpected
	{
		ErrorT error;
	};

	template <typename ErrorT>
	Unexpected(ErrorT) -> Unexpected<ErrorT>;

	/**
	 * @brief Represents a value-or-error result without dynamic allocation.
	 *
	 * @tparam ValueT The success value type.
	 * @tparam ErrorT The error type.
	 *
	 * @details
	 * Expected is a lightweight alternative to exception-based error handling.
	 * An Expected object contains either:
	 * - A valid value of type ValueT, or
	 * - An error value of type ErrorT
	 *
	 * At any time, exactly one of these is active.
	 *
	 * This implementation:
	 * - Performs no dynamic allocations
	 * - Uses explicit lifetime management
	 * - Is suitable for low-level and performance-critical code
	 *
	 * @note Users must check @ref has_value() before calling @ref value() or
	 *       @ref error().
	 */
	template <typename ValueT, typename ErrorT>
	class [[nodiscard]] Expected
	{
	public:

		using ValueType = ValueT;
		using ErrorType = ErrorT;

		Expected(const ValueT& value) noexcept(std::is_nothrow_copy_constructible_v<ValueT>)
			requires std::is_copy_constructible_v<ValueT>
			: m_hasValue(true)
		{
			new(&m_storage.value) ValueT(value);
		}

		Expected(ValueType&& value) noexcept(std::is_nothrow_move_constructible_v<ValueType>) : m_hasValue(true)
		{
			new(&m_storage.value) ValueType(std::move(value));
		}

		Expected(Unexpected<ErrorT> unexpected) noexcept(std::is_nothrow_move_constructible_v<ErrorT>) : m_hasValue(false)
		{
			new(&m_storage.error) ErrorT(std::move(unexpected.error));
		}

		Expected(const Expected& other)
			requires(std::is_copy_constructible_v<ValueT> && std::is_copy_constructible_v<ErrorT>)
			: m_hasValue(other.m_hasValue)
		{
			if(m_hasValue)
			{
				new(&m_storage.value) ValueT(other.m_storage.value);
			}
			else
			{
				new(&m_storage.error) ErrorT(other.m_storage.error);
			}
		}

		Expected(Expected&& other) noexcept(std::is_nothrow_move_constructible_v<ValueT> && std::is_nothrow_move_constructible_v<ErrorT>) :
			m_hasValue(other.m_hasValue)
		{
			if(m_hasValue)
			{
				new(&m_storage.value) ValueT(std::move(other.m_storage.value));
			}
			else
			{
				new(&m_storage.error) ErrorT(std::move(other.m_storage.error));
			}
		}

		~Expected() { destroy(); }

		/**
		 * @brief Returns whether the Expected contains a valid value.
		 */
		[[nodiscard]] bool has_value() const noexcept { return m_hasValue; }

		/**
		 * @brief Implicit check for success.
		 *
		 * Equivalent to calling @ref has_value().
		 */
		explicit operator bool() const noexcept { return has_value(); }

		/**
		 * @brief Accesses the contained value.
		 *
		 * @pre has_value() == true
		 */
		ValueT& value()
		{
			assert(m_hasValue);
			return m_storage.value;
		}

		/**
		 * @brief Accesses the contained value (const).
		 *
		 * @pre has_value() == true
		 */
		const ValueT& value() const
		{
			assert(m_hasValue);
			return m_storage.value;
		}

		/**
		 * @brief Accesses the contained error.
		 *
		 * @pre has_value() == false
		 */
		ErrorT& error()
		{
			assert(!m_hasValue);
			return m_storage.error;
		}

		/**
		 * @brief Accesses the contained error (const).
		 *
		 * @pre has_value() == false
		 */
		const ErrorT& error() const
		{
			assert(!m_hasValue);
			return m_storage.error;
		}

		/**
		 * @brief Returns the contained value or a fallback.
		 *
		 * @param fallback Value to return if this Expected contains an error.
		 */
		ValueT value_or(ValueT fallback) const { return m_hasValue ? m_storage.value : fallback; }

	private:

		void destroy() noexcept(std::is_nothrow_destructible_v<ValueT> && std::is_nothrow_destructible_v<ErrorT>)
		{
			if(m_hasValue)
			{
				m_storage.value.~ValueT();
			}
			else
			{
				m_storage.error.~ErrorT();
			}
		}

	private:

		union Storage
		{
			ValueT value;
			ErrorT error;

			Storage() {}
			~Storage() {}
		} m_storage;

		bool m_hasValue;
	};

	// Overload for Expected<void, ErrorT>

	/**
	 * @brief Specialization of Expected for operations that return no value.
	 *
	 * @tparam ErrorT The error type.
	 *
	 * @details
	 * This specialization represents either:
	 * - Success (no value), or
	 * - Failure with an error
	 *
	 * It is useful for functions that conceptually return `void` on success
	 * but may fail.
	 */
	template <typename ErrorT>
	class [[nodiscard]] Expected<void, ErrorT>
	{
	public:

		/**
		 * @brief Constructs a successful result.
		 */
		Expected() noexcept : m_hasValue(true) {}

		/**
		 * @brief Constructs a failure result.
		 *
		 * @param unexpected Wrapper containing the error.
		 */
		Expected(Unexpected<ErrorT> unexpected) : m_hasValue(false) { new(&m_error) ErrorT(std::move(unexpected.error)); }

		~Expected()
		{
			if(!m_hasValue)
			{
				m_error.~ErrorT();
			}
		}

		/**
		 * @brief Returns whether the operation succeeded.
		 */
		[[nodiscard]] bool has_value() const noexcept { return m_hasValue; }

		/**
		 * @brief Checks success.
		 */
		explicit operator bool() const noexcept { return has_value(); }

		/**
		 * @brief Asserts success.
		 *
		 * @pre has_value() == true
		 */
		void value() const { assert(m_hasValue); }

		/**
		 * @brief Accesses the stored error.
		 *
		 * @pre has_value() == false
		 */
		ErrorT& error()
		{
			assert(!m_hasValue);
			return m_error;
		}

		/**
		 * @brief Accesses the stored error. (const)
		 *
		 * @pre has_value() == false
		 */
		const ErrorT& error() const
		{
			assert(!m_hasValue);
			return m_error;
		}

	private:

		union
		{
			ErrorT m_error;
		};

		bool m_hasValue;
	};

} // namespace opus3d