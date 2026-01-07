#pragma once

#include <foundation/core/include/assert.hpp>
#include <foundation/core/include/result.hpp>

#include "memory_error.hpp"

#include <memory>
#include <type_traits>

namespace opus3d::foundation::memory
{
	class Allocator
	{
	public:

		using AllocateFn   = Result<void*> (*)(void* ctx, size_t size, size_t alignment) noexcept;
		using DeallocateFn = void (*)(void* ctx, void* ptr, size_t size, size_t alignment) noexcept;
		using ResizeFn	   = Result<void*> (*)(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t alignment) noexcept;

		Allocator(void* context, AllocateFn allocFn, DeallocateFn deallocFn, ResizeFn resizeFn = nullptr) :
			m_context(context), m_allocateFn(allocFn), m_deallocateFn(deallocFn), m_resizeFn(resizeFn)
		{}

		Allocator(const Allocator&) = default;

		[[nodiscard]] Result<void*> try_allocate(size_t size, size_t alignment) noexcept
		{
			ASSERT_MSG(m_allocateFn, "Allocator is missing m_allocateFn");
			return m_allocateFn(m_context, size, alignment);
		}

		[[nodiscard]] void* allocate(size_t size, size_t alignment) noexcept
		{
			Result r = try_allocate(size, alignment);
			ASSERT_MSG(r.has_value(), "Out of memory");
			return r.value();
		}

		void deallocate(void* ptr, size_t size, size_t alignment) noexcept
		{
			ASSERT_MSG(m_deallocateFn, "Allocator is missing deallocate_fn");
			m_deallocateFn(m_context, ptr, size, alignment);
		}

		[[nodiscard]] Result<void*> try_resize(void* ptr, size_t oldSize, size_t newSize, size_t alignment) noexcept
		{
			if(!m_resizeFn)
			{
				return Unexpected(ErrorCode::create(error_domains::Memory, static_cast<uint32_t>(MemoryErrorCode::AllocatorNoResize)));
			}
			return m_resizeFn(m_context, ptr, oldSize, newSize, alignment);
		}

		template <typename T, typename... Args>
		[[nodiscard]] T* make(Args&&... args) noexcept
		{
			static_assert(std::is_nothrow_constructible_v<T, Args...>, "make requires nothrow-constructible T");

			T* p = reinterpret_cast<T*>(allocate(sizeof(T), alignof(T)));
			// allocate panics on failure.
			std::construct_at(p, std::forward<Args>(args)...);

			return p;
		}

		template <typename T, typename... Args>
		[[nodiscard]] Result<T*> try_make(Args&&... args) noexcept
		{
			static_assert(std::is_nothrow_constructible_v<T, Args...>, "try_make requires nothrow-constructible T");

			if(Result<T*> alloc = try_allocate(sizeof(T), alignof(T)); alloc.has_value())
			{
				T* ptr = reinterpret_cast<T*>(alloc.value());
				std::construct_at(ptr, std::forward<Args>(args)...);
				return ptr;
			}
			else
			{
				return Unexpected(alloc.error());
			}
		}

		template <typename T>
		void destroy(T* ptr) noexcept
		{
			std::destroy_at(ptr);
			deallocate(ptr, sizeof(T), alignof(T));
		}

		template <typename T, typename... Args>
		[[nodiscard]] T* make_range(size_t count, Args&&... args) noexcept
		{
			static_assert(std::is_nothrow_constructible_v<T, Args...>, "allocate_range requires nothrow-constructible T");

			if(count == 0)
			{
				return nullptr;
			}

			T* arr = static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
			if constexpr(!std::is_trivially_constructible_v<T, Args...>)
			{
				for(size_t i = 0; i < count; ++i)
				{
					std::construct_at(arr + i, std::forward<Args>(args)...);
				}
			}
			return arr;
		}

		template <typename T>
		void destroy_range(T* ptr, size_t count) noexcept
		{
			static_assert(std::is_nothrow_destructible_v<T>, "free_range requires nothrow-destructible T");

			if(count == 0)
			{
				return;
			}

			if constexpr(!std::is_trivially_destructible_v<T>)
			{
				for(size_t i = count; i-- > 0;)
				{
					std::destroy_at(ptr + i);
				}
			}
			deallocate(ptr, sizeof(T) * count, alignof(T));
		}

		// Comparison, useful for fast paths in containers.

		bool operator==(const Allocator& rhs) const noexcept { return m_context == rhs.m_context; }
		bool operator!=(const Allocator& rhs) const noexcept { return m_context != rhs.m_context; }

	private:

		void*	     m_context	    = nullptr;
		AllocateFn   m_allocateFn   = nullptr;
		DeallocateFn m_deallocateFn = nullptr;
		ResizeFn     m_resizeFn	    = nullptr;
	};

	// Unique ptr deleter + typedef

	template <typename T>
	struct AllocatorDeleter
	{
		Allocator* allocator = nullptr;

		void operator()(T* ptr) const noexcept
		{
			std::destroy_at(ptr);
			allocator->deallocate(ptr, sizeof(T), alignof(T));
		}
	};

	template <typename T>
	using UniquePtr = std::unique_ptr<T, AllocatorDeleter<T>>;

	template <typename T, typename... Args>
	UniquePtr<T> make_unique(Allocator& allocator, Args&&... args)
	{
		return UniquePtr<T>{allocator.make<T>(std::forward<Args>(args)...), AllocatorDeleter<T>{&allocator}};
	}

	template <typename T, typename... Args>
	Result<UniquePtr<T>> try_make_unique(Allocator& allocator, Args&&... args)
	{
		return UniquePtr<T>{allocator.make<T>(std::forward<Args>(args)...), AllocatorDeleter<T>{&allocator}};
	}

} // namespace opus3d::foundation::memory