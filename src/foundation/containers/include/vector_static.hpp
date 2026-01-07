#pragma once

#include <foundation/core/include/assert.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace opus3d::foundation
{
	template <typename T, size_t N>
	class VectorStatic
	{
	public:

		using ValueType = T;

		constexpr VectorStatic() noexcept = default;

		VectorStatic(const VectorStatic& v) noexcept
			requires std::is_copy_constructible_v<T>;

		VectorStatic(VectorStatic&& v) noexcept
			requires std::is_move_constructible_v<T>;

		VectorStatic(std::initializer_list<T> init);
		~VectorStatic() noexcept;

		void clear() noexcept;

		template <typename... Args>
		T& emplace_back(Args&&... args) noexcept
			requires std::is_constructible_v<T, Args...>;

		void push_back(const T& t) noexcept;
		void push_back(T&& t) noexcept;

		void erase_unordered(size_t i) noexcept;
		void erase_ordered(size_t i) noexcept;

		void pop_back() noexcept;

		T& front() noexcept;

		const T& front() const noexcept;
		T&	 back() noexcept;

		const T& back() const noexcept;
		T&	 operator[](const size_t i) noexcept;
		const T& operator[](const size_t i) const noexcept;

		constexpr VectorStatic& operator=(VectorStatic&& rhs) noexcept
			requires std::is_nothrow_move_constructible_v<T>;

		constexpr VectorStatic& operator=(const VectorStatic& rhs) noexcept
			requires std::is_copy_constructible_v<T>;

		// --- Finders ---

		T*	 find(const T& value) noexcept;
		const T* find(const T& value) const noexcept;

		template <typename Pred>
			requires std::predicate<Pred, const T&>
		constexpr std::optional<size_t> find_if(Pred&& pred) const noexcept;

		// --- Accessors ---

		T&	 at(size_t i) noexcept;
		const T& at(size_t i) const noexcept;

		T*	 begin() noexcept;
		const T* begin() const noexcept;

		T*	 end() noexcept;
		const T* end() const noexcept;

		T*	 data() noexcept;
		const T* data() const noexcept;

		[[nodiscard]] bool   empty() const noexcept;
		[[nodiscard]] bool   full() const noexcept;
		[[nodiscard]] size_t size() const noexcept;
		[[nodiscard]] size_t capacity() const noexcept;

		// --- Views ---

		[[nodiscard]] std::span<T>	 view() noexcept;
		[[nodiscard]] std::span<const T> view() const noexcept;

		std::span<std::byte>	   as_bytes() noexcept;
		std::span<const std::byte> as_bytes() const noexcept;

	private:

		size_t m_size = 0;
		alignas(T) std::byte m_storage[N * sizeof(T)];
	};

	template <typename T, size_t N>
	VectorStatic<T, N>::VectorStatic(const VectorStatic& v) noexcept
		requires std::is_copy_constructible_v<T>
		: m_size(v.m_size)
	{
		static_assert(std::is_nothrow_copy_constructible_v<T>, "VectorStatic is not designed for exceptions!");

		for(size_t i = 0; i < m_size; ++i)
		{
			std::construct_at(data() + i, v[i]);
		}
	}

	template <typename T, size_t N>
	VectorStatic<T, N>::VectorStatic(VectorStatic&& v) noexcept
		requires std::is_move_constructible_v<T>
		: m_size(std::exchange(v.m_size, 0))
	{
		static_assert(std::is_nothrow_move_constructible_v<T>, "VectorStatic is not designed for exceptions!");

		for(size_t i = 0; i < m_size; ++i)
		{
			std::construct_at(data() + i, std::move(v[i]));
		}
	}

	template <typename T, size_t N>
	VectorStatic<T, N>::VectorStatic(std::initializer_list<T> init)
	{
		ASSERT(init.size() <= N);
		static_assert(std::is_nothrow_copy_constructible_v<T>, "VectorStatic: T must be nothrow copy constructible");

		std::uninitialized_copy(init.begin(), init.end(), data());
		m_size = init.size();
	}

	template <typename T, size_t N>
	VectorStatic<T, N>::~VectorStatic() noexcept
	{
		clear();
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::clear() noexcept
	{
		if constexpr(!std::is_trivially_destructible_v<T>)
		{
			std::destroy_n(data(), m_size);
		}
		m_size = 0;
	}

	template <typename T, size_t N>
	template <typename... Args>
	T& VectorStatic<T, N>::emplace_back(Args&&... args) noexcept
		requires std::is_constructible_v<T, Args...>
	{
		DEBUG_ASSERT_MSG(m_size < N, "VectorStatic full!");
		std::construct_at(data() + m_size, std::forward<Args>(args)...);
		return (*this)[m_size++];
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::push_back(const T& t) noexcept
	{
		T temp = t; // protects against self-aliasing
		emplace_back(std::move(temp));
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::push_back(T&& t) noexcept
	{
		T temp = std::move(t); // protects against self-aliasing
		emplace_back(std::move(temp));
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::erase_unordered(size_t i) noexcept
	{
		static_assert(std::is_nothrow_move_assignable_v<T> || std::is_trivially_copyable_v<T> || std::is_nothrow_swappable_v<T>,
			      "erase_unordered requires nothrow move-assign, trivial copy, or nothrow swap");

		DEBUG_ASSERT_MSG(i < m_size, "VectorStatic erase_ordered index out of bounds!");

		if(i != m_size - 1)
		{
			(*this)[i] = std::move((*this)[m_size - 1]);
		}

		pop_back();
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::erase_ordered(size_t i) noexcept
	{
		static_assert(std::is_nothrow_move_assignable_v<T> || std::is_trivially_copyable_v<T>, "erase_ordered requires nothrow move or trivial copy");

		DEBUG_ASSERT_MSG(i < m_size, "Index out of bounds!");

		// Shift elements left
		if constexpr(std::is_trivially_copyable_v<T>)
		{
			// memmove handles overlapping regions safely
			if(i + 1 < m_size)
			{
				std::memmove(data() + i, data() + i + 1, (m_size - i - 1) * sizeof(T));
			}
		}
		else
		{
			for(size_t j = i; j + 1 < m_size; ++j)
			{
				(*this)[j] = std::move((*this)[j + 1]);
			}
		}
		pop_back();
	}

	template <typename T, size_t N>
	void VectorStatic<T, N>::pop_back() noexcept
	{
		DEBUG_ASSERT_MSG(m_size > 0, "VectorStatic empty!");

		if constexpr(!std::is_trivially_destructible_v<T>)
		{
			std::destroy_at(m_size - 1);
		}
		--m_size;
	}

	template <typename T, size_t N>
	T& VectorStatic<T, N>::front() noexcept
	{
		DEBUG_ASSERT_MSG(m_size > 0, "VectorStatic empty during front()!");
		return data()[0];
	}

	template <typename T, size_t N>
	const T& VectorStatic<T, N>::front() const noexcept
	{
		DEBUG_ASSERT_MSG(m_size > 0, "VectorStatic empty during front()!");
		return data()[0];
	}

	template <typename T, size_t N>
	T& VectorStatic<T, N>::back() noexcept
	{
		DEBUG_ASSERT_MSG(m_size > 0, "VectorStatic empty during back()!");
		return data()[m_size - 1];
	}

	template <typename T, size_t N>
	const T& VectorStatic<T, N>::back() const noexcept
	{
		DEBUG_ASSERT_MSG(m_size > 0, "VectorStatic empty during back()!");
		return data()[m_size - 1];
	}

	template <typename T, size_t N>
	T& VectorStatic<T, N>::operator[](const size_t i) noexcept
	{
		DEBUG_ASSERT(i < m_size);
		return data()[i];
	}

	template <typename T, size_t N>
	const T& VectorStatic<T, N>::operator[](const size_t i) const noexcept
	{
		DEBUG_ASSERT(i < m_size);
		return data()[i];
	}

	template <typename T, size_t N>
	constexpr VectorStatic<T, N>& VectorStatic<T, N>::operator=(VectorStatic&& rhs) noexcept
		requires std::is_nothrow_move_constructible_v<T>
	{
		if(this != &rhs)
		{
			const size_t newSize = rhs.m_size;
			if constexpr(std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>)
			{
				std::memcpy(m_storage, rhs.m_storage, newSize * sizeof(T));
			}
			else
			{
				const size_t common = std::min(m_size, newSize);

				// Move Assign.
				for(size_t i = 0; i < common; ++i)
				{
					(*this)[i] = std::move(rhs[i]);
				}

				// Move Construct.
				if(newSize > m_size)
				{
					std::uninitialized_move_n(rhs.data() + m_size, newSize - m_size, data() + m_size);
				}
				// Destroy excess.
				else if(m_size > newSize)
				{
					std::destroy_n(data() + newSize, m_size - newSize);
				}
			}
			m_size = newSize;
			rhs.clear();
		}
		return *this;
	}

	template <typename T, size_t N>
	constexpr VectorStatic<T, N>& VectorStatic<T, N>::operator=(const VectorStatic& rhs) noexcept
		requires std::is_copy_constructible_v<T>
	{
		if(this != &rhs)
		{
			const size_t newSize = rhs.m_size;
			if constexpr(std::is_trivially_copyable_v<T>)
			{
				std::memcpy(m_storage, rhs.m_storage, newSize * sizeof(T));
			}
			else
			{
				const size_t common = std::min(m_size, newSize);

				// Assign to existing.
				for(size_t i = 0; i < common; ++i)
				{
					(*this)[i] = rhs[i];
				}

				// Construct new.
				if(newSize > m_size)
				{
					std::uninitialized_copy_n(rhs.data() + m_size, newSize - m_size, data() + m_size);
				}
				// Destroy excess.
				else if(m_size > newSize)
				{
					std::destroy_n(data() + newSize, m_size - newSize);
				}
			}
			m_size = newSize;
		}
		return *this;
	}

	template <typename T, size_t N>
	T* VectorStatic<T, N>::find(const T& value) noexcept
	{
		for(size_t i = 0; i < m_size; ++i)
		{
			if(data()[i] == value)
			{
				return &data()[i];
			}
		}
		return nullptr;
	}

	template <typename T, size_t N>
	const T* VectorStatic<T, N>::find(const T& value) const noexcept
	{
		for(size_t i = 0; i < m_size; ++i)
		{
			if(data()[i] == value)
			{
				return &data()[i];
			}
		}
		return nullptr;
	}

	template <typename T, size_t N>
	template <typename Pred>
		requires std::predicate<Pred, const T&>
	constexpr std::optional<size_t> VectorStatic<T, N>::find_if(Pred&& pred) const noexcept
	{
		for(size_t i = 0; i < m_size; ++i)
		{
			if(pred(data()[i]))
			{
				return i;
			}
		}
		return std::nullopt;
	}

	template <typename T, size_t N>
	T& VectorStatic<T, N>::at(size_t i) noexcept
	{
		ASSERT(i < m_size);
		return (*this)[i];
	}

	template <typename T, size_t N>
	const T& VectorStatic<T, N>::at(size_t i) const noexcept
	{
		ASSERT(i < m_size);
		return (*this)[i];
	}

	template <typename T, size_t N>
	T* VectorStatic<T, N>::begin() noexcept
	{
		return data();
	}

	template <typename T, size_t N>
	const T* VectorStatic<T, N>::begin() const noexcept
	{
		return data();
	}

	template <typename T, size_t N>
	T* VectorStatic<T, N>::end() noexcept
	{
		return data() + m_size;
	}

	template <typename T, size_t N>
	const T* VectorStatic<T, N>::end() const noexcept
	{
		return data() + m_size;
	}

	template <typename T, size_t N>
	T* VectorStatic<T, N>::data() noexcept
	{
		return std::launder(reinterpret_cast<T*>(m_storage));
	}

	template <typename T, size_t N>
	const T* VectorStatic<T, N>::data() const noexcept
	{
		return std::launder(reinterpret_cast<const T*>(m_storage));
	}

	template <typename T, size_t N>
	[[nodiscard]] bool VectorStatic<T, N>::empty() const noexcept
	{
		return m_size == 0;
	}

	template <typename T, size_t N>
	[[nodiscard]] bool VectorStatic<T, N>::full() const noexcept
	{
		return m_size == N;
	}

	template <typename T, size_t N>
	[[nodiscard]] size_t VectorStatic<T, N>::size() const noexcept
	{
		return m_size;
	}

	template <typename T, size_t N>
	[[nodiscard]] size_t VectorStatic<T, N>::capacity() const noexcept
	{
		return N;
	}

	template <typename T, size_t N>
	[[nodiscard]] std::span<T> VectorStatic<T, N>::view() noexcept
	{
		return {data(), m_size};
	}

	template <typename T, size_t N>
	[[nodiscard]] std::span<const T> VectorStatic<T, N>::view() const noexcept
	{
		return {data(), m_size};
	}

	template <typename T, size_t N>
	std::span<std::byte> VectorStatic<T, N>::as_bytes() noexcept
	{
		return {reinterpret_cast<std::byte*>(data()), m_size * sizeof(T)};
	}

	template <typename T, size_t N>
	std::span<const std::byte> VectorStatic<T, N>::as_bytes() const noexcept
	{
		return {reinterpret_cast<const std::byte*>(data()), m_size * sizeof(T)};
	}

} // namespace opus3d::foundation