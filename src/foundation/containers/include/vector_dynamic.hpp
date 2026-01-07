#pragma once

#include <foundation/core/include/assert.hpp>
#include <foundation/core/include/result.hpp>

#include <foundation/memory/include/allocator.hpp>
#include <memory>
#include <optional>

namespace opus3d::foundation
{
	// A VectorDynamic CANNOT exist without an allocator.
	// A VectorDynamic MUST NOT outlive its allocator.

	template <typename T>
	class VectorDynamic
	{
	public:

		VectorDynamic(Allocator alloc) noexcept;

		VectorDynamic(const VectorDynamic& rhs) noexcept;

		VectorDynamic(VectorDynamic&& rhs) noexcept;

		~VectorDynamic() noexcept;

		VectorDynamic& operator=(VectorDynamic&& rhs) noexcept;
		VectorDynamic& operator=(const VectorDynamic& rhs) noexcept;

		template <typename... Args>
		T& emplace_back(Args&&... args);

		void push_back(const T& value);

		void push_back(T&& value);

		// A variant of push_back that returns a Result in case of allocation failure.
		[[nodiscard]] Result<void> try_push_back(const T& value);

		void pop_back() noexcept;

		// Resize container to contain n elements.
		// If n < size, elements are truncated.
		// If n > size, default-inserted elements are appended.
		void resize(size_t n) noexcept;

		void erase_unordered(size_t index) noexcept;

		void reserve(size_t n) noexcept;

		[[nodiscard]] Result<void> try_reserve(size_t n) noexcept;

		void clear() noexcept;

		// shrink_to_fit()
		// Attempts to reduce capacity to exactly size().
		// This is an explicit, potentially expensive operation.
		// If the allocator supports resize, shrinking may occur in place.
		// Otherwise, this may allocate and move elements.
		// Failure is non-fatal and leaves the vector unchanged.
		Result<void> shrink_to_fit() noexcept;

		T& operator[](size_t i) noexcept;

		const T& operator[](size_t i) const noexcept;
		T&	 at(size_t i) noexcept;
		const T& at(size_t i) const noexcept;

		std::optional<size_t> find(const T& value) const noexcept;

		template <typename Pred>
			requires std::predicate<Pred, const T&>
		std::optional<size_t> find_if(Pred&& pred) noexcept;

		bool empty() const noexcept;

		size_t size() const noexcept;

		size_t capacity() const noexcept;

		T*	 data() noexcept;
		const T* data() const noexcept;

		T*	 begin() noexcept;
		const T* begin() const noexcept;

		T*	 end() noexcept;
		const T* end() const noexcept;

	private:

		T*	   allocate_objects(size_t n) noexcept;
		Result<T*> try_allocate_objects(size_t n) noexcept;

		void	     grow_capacity();
		Result<void> try_grow_capacity();

	private:

		Allocator m_allocator;
		T*	  m_data     = nullptr;
		size_t	  m_size     = 0;
		size_t	  m_capacity = 0;
	};

	template <typename T>
	VectorDynamic<T>::VectorDynamic(Allocator alloc) noexcept : m_allocator(alloc)
	{}

	template <typename T>
	VectorDynamic<T>::VectorDynamic(const VectorDynamic& rhs) noexcept : m_allocator(rhs.m_allocator)
	{
		static_assert(std::is_nothrow_copy_constructible_v<T>, "VectorDynamic does not support exceptions!");

		const size_t objectCount = rhs.size();
		if(objectCount > 0)
		{
			m_data = allocate_objects(objectCount);

			if constexpr(std::is_trivially_copy_constructible_v<T>)
			{
				std::memcpy(m_data, rhs.m_data, sizeof(T) * objectCount);
			}
			else
			{
				for(size_t i = 0; i < objectCount; ++i)
				{
					std::construct_at(m_data + i, rhs.m_data[i]);
				}
			}
			m_size	   = objectCount;
			m_capacity = objectCount;
		}
	}

	template <typename T>
	VectorDynamic<T>::VectorDynamic(VectorDynamic&& rhs) noexcept :
		m_allocator(rhs.m_allocator), m_data(rhs.m_data), m_size(rhs.m_size), m_capacity(rhs.m_capacity)
	{
		rhs.m_data     = nullptr;
		rhs.m_size     = 0;
		rhs.m_capacity = 0;
	}

	template <typename T>
	VectorDynamic<T>::~VectorDynamic() noexcept
	{
		clear();
	}

	template <typename T>
	VectorDynamic<T>& VectorDynamic<T>::operator=(VectorDynamic&& rhs) noexcept
	{
		if(this != &rhs)
		{
			clear();

			if(m_allocator == rhs.m_allocator)
			{
				// Fast path: same allocator -> steal buffer
				if(m_data)
				{
					m_allocator.deallocate(m_data, sizeof(T) * m_capacity, alignof(T));
				}

				m_data	   = rhs.m_data;
				m_size	   = rhs.m_size;
				m_capacity = rhs.m_capacity;

				rhs.m_data     = nullptr;
				rhs.m_size     = 0;
				rhs.m_capacity = 0;
			}
			else
			{
				// Different allocator -> element-wise move
				reserve(rhs.m_size);

				for(size_t i = 0; i < rhs.m_size; ++i)
				{
					std::construct_at(m_data + i, std::move(rhs.m_data[i]));
				}

				m_size = rhs.m_size;
			}
		}
		return *this;
	}

	template <typename T>
	VectorDynamic<T>& VectorDynamic<T>::operator=(const VectorDynamic& rhs) noexcept
	{
		if(this != &rhs)
		{
			clear();

			if(rhs.m_size > 0)
			{
				reserve(rhs.m_size);

				if constexpr(std::is_trivially_copyable_v<T>)
				{
					std::memcpy(m_data, rhs.m_data, sizeof(T) * rhs.m_size);
				}
				else
				{
					for(size_t i = 0; i < rhs.m_size; ++i)
					{
						std::construct_at(m_data + i, rhs[i]);
					}
				}

				m_size = rhs.m_size;
			}
		}
		return *this;
	}

	template <typename T>
	template <typename... Args>
	T& VectorDynamic<T>::emplace_back(Args&&... args)
	{
		if(m_size == m_capacity)
		{
			grow_capacity();
		}

		// Construct in-place at the end
		std::construct_at(m_data + m_size, std::forward<Args>(args)...);
		return m_data[m_size++];
	}

	template <typename T>
	void VectorDynamic<T>::push_back(const T& value)
	{
		emplace_back(value);
	}

	template <typename T>
	void VectorDynamic<T>::push_back(T&& value)
	{
		emplace_back(std::move(value));
	}

	template <typename T>
	[[nodiscard]] Result<void> VectorDynamic<T>::try_push_back(const T& value)
	{
		if(m_size == m_capacity)
		{
			if(Result res = try_grow_capacity(); !res.has_value())
			{
				return res;
			}
		}
		std::construct_at(m_data + m_size, value);
		m_size++;
		return {};
	}

	template <typename T>
	void VectorDynamic<T>::pop_back() noexcept
	{
		DEBUG_ASSERT(m_size > 0);
		if constexpr(!std::is_trivially_destructible_v<T>)
		{
			std::destroy_at(m_data + m_size - 1);
		}
		--m_size;
	}

	template <typename T>
	void VectorDynamic<T>::resize(size_t n) noexcept
	{
		if(n < m_size)
		{
			if constexpr(!std::is_trivially_destructible_v<T>)
			{
				std::destroy(m_data + n, m_data + m_size);
			}
			m_size = n;
		}
		else if(n > m_size)
		{
			reserve(n);
			if constexpr(std::is_trivially_default_constructible_v<T>)
			{
				// For trivial types (int, float), we technically don't need to zero-init
				// to match std::vector semantics, but let's default construct.
				std::memset(m_data + m_size, 0, (n - m_size) * sizeof(T));
			}
			else
			{
				std::uninitialized_default_construct(m_data + m_size, m_data + n);
			}
			m_size = n;
		}
	}

	template <typename T>
	void VectorDynamic<T>::erase_unordered(size_t index) noexcept
	{
		DEBUG_ASSERT(index < m_size);
		if(index != m_size - 1)
		{
			m_data[index] = std::move(m_data[m_size - 1]);
		}
		pop_back();
	}

	template <typename T>
	void VectorDynamic<T>::reserve(size_t n) noexcept
	{
		Result<void> r = try_reserve(n);
		ASSERT_MSG(r.has_value(), "Out of memory");
	}

	template <typename T>
	[[nodiscard]] Result<void> VectorDynamic<T>::try_reserve(size_t n) noexcept
	{
		static_assert(std::is_nothrow_move_constructible_v<T>, "VectorDynamic does not support exceptions!");

		if(n > m_capacity)
		{
			Result<T*> newBlock = try_allocate_objects(n);
			if(!newBlock.has_value())
			{
				return Unexpected(newBlock.error());
			}

			if(m_size > 0)
			{
				if constexpr(std::is_trivially_copyable_v<T>)
				{
					std::memcpy(newBlock.value(), m_data, sizeof(T) * m_size);
				}
				else
				{
					for(size_t i = 0; i < m_size; ++i)
					{
						std::construct_at(newBlock.value() + i, std::move(m_data[i]));
					}
				}

				if constexpr(!std::is_trivially_destructible_v<T>)
				{
					for(size_t i = 0; i < m_size; ++i)
					{
						std::destroy_at(m_data + i);
					}
				}
			}

			if(m_data)
			{
				m_allocator.deallocate(m_data, sizeof(T) * m_capacity, alignof(T));
			}

			m_data	   = newBlock.value();
			m_capacity = n;
		}

		return {};
	}

	template <typename T>
	void VectorDynamic<T>::clear() noexcept
	{
		static_assert(std::is_nothrow_destructible_v<T>, "VectorDynamic does not support exceptions!");

		if constexpr(!std::is_trivially_destructible_v<T>)
		{
			std::destroy_n(m_data, m_size);
		}
		m_size = 0;
	}

	template <typename T>
	Result<void> VectorDynamic<T>::shrink_to_fit() noexcept
	{
		if(m_size == m_capacity)
		{
			return {};
		}

		const size_t oldBytes = sizeof(T) * m_capacity;
		const size_t newBytes = sizeof(T) * m_size;

		// Try in-place resize if supported:
		if(m_data && newBytes > 0)
		{
			if(Result<void*> r = m_allocator.try_resize(m_data, oldBytes, newBytes, alignof(T)); r.has_value())
			{
				m_data	   = static_cast<T*>(r.value());
				m_capacity = m_size;
				return {};
			}
		}

		// Fallback: allocate-copy-free
		if(m_size == 0)
		{
			m_allocator.deallocate(m_data, oldBytes, alignof(T));
			m_data	   = nullptr;
			m_capacity = 0;
			return {};
		}

		Result<T*> newBlock = try_allocate_objects(m_size);
		if(!newBlock.has_value())
		{
			return Unexpected(newBlock.error());
		}

		T* newData = newBlock.value();

		if constexpr(std::is_trivially_move_constructible_v<T>)
		{
			std::memcpy(newData, m_data, newBytes);
		}
		else
		{
			for(size_t i = 0; i < m_size; ++i)
			{
				std::construct_at(newData + i, std::move(m_data[i]));
			}

			if constexpr(!std::is_trivially_destructible_v<T>)
			{
				std::destroy_n(m_data, m_size);
			}
		}

		m_allocator.deallocate(m_data, oldBytes, alignof(T));

		m_data	   = newData;
		m_capacity = m_size;
		return {};
	}

	template <typename T>
	T& VectorDynamic<T>::operator[](size_t i) noexcept
	{
		DEBUG_ASSERT(i < m_size);
		return m_data[i];
	}

	template <typename T>
	const T& VectorDynamic<T>::operator[](size_t i) const noexcept
	{
		DEBUG_ASSERT(i < m_size);
		return m_data[i];
	}

	template <typename T>
	T& VectorDynamic<T>::at(size_t i) noexcept
	{
		ASSERT(i < m_size);
		return m_data[i];
	}

	template <typename T>
	const T& VectorDynamic<T>::at(size_t i) const noexcept
	{
		ASSERT(i < m_size);
		return m_data[i];
	}

	template <typename T>
	std::optional<size_t> VectorDynamic<T>::find(const T& value) const noexcept
	{
		for(size_t i = 0; i < m_size; ++i)
		{
			if(data()[i] == value)
			{
				return i;
			}
		}
		return std::nullopt;
	}

	template <typename T>
	template <typename Pred>
		requires std::predicate<Pred, const T&>
	std::optional<size_t> VectorDynamic<T>::find_if(Pred&& pred) noexcept
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

	template <typename T>
	bool VectorDynamic<T>::empty() const noexcept
	{
		return m_size == 0;
	}

	template <typename T>
	size_t VectorDynamic<T>::size() const noexcept
	{
		return m_size;
	}

	template <typename T>
	size_t VectorDynamic<T>::capacity() const noexcept
	{
		return m_capacity;
	}

	template <typename T>
	T* VectorDynamic<T>::data() noexcept
	{
		return m_data;
	}

	template <typename T>
	const T* VectorDynamic<T>::data() const noexcept
	{
		return m_data;
	}

	template <typename T>
	T* VectorDynamic<T>::begin() noexcept
	{
		return m_data;
	}

	template <typename T>
	const T* VectorDynamic<T>::begin() const noexcept
	{
		return m_data;
	}

	template <typename T>
	T* VectorDynamic<T>::end() noexcept
	{
		return m_data + m_size;
	}

	template <typename T>
	const T* VectorDynamic<T>::end() const noexcept
	{
		return m_data + m_size;
	}

	template <typename T>
	T* VectorDynamic<T>::allocate_objects(size_t n) noexcept
	{
		return m_allocator.allocate(sizeof(T) * n, alignof(T));
	}

	template <typename T>
	Result<T*> VectorDynamic<T>::try_allocate_objects(size_t n) noexcept
	{
		// MSVC can't handle Result without <void*>
		if(Result<void*> alloc = m_allocator.try_allocate(sizeof(T) * n, alignof(T)); alloc.has_value())
		{
			return static_cast<T*>(alloc.value());
		}
		else
		{
			return Unexpected(alloc.error());
		}
	}

	template <typename T>
	void VectorDynamic<T>::grow_capacity()
	{
		size_t new_cap = (m_capacity == 0) ? 8 : m_capacity * 2;
		reserve(new_cap);
	}

	template <typename T>
	Result<void> VectorDynamic<T>::try_grow_capacity()
	{
		size_t new_cap = (m_capacity == 0) ? 8 : m_capacity * 2;
		return try_reserve(new_cap);
	}

} // namespace opus3d::foundation