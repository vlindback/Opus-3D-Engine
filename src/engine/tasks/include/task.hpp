// task.hpp
// Copyright 2025 Viktor Lindbäck. Licensed under the MIT License.

// std::function like wrapper for tasks.

#include <cassert>
#include <memory>
#include <type_traits>

namespace opus3d
{
	class Task
	{

	public:

		Task() = default;

		template <typename F, typename IsNotTask = std::enable_if_t<!std::is_same_v<std::decay_t<F>, Task>>>
		Task(F&& func) noexcept;

		Task(const Task& other);

		Task(Task&& other);

		~Task();

		explicit operator bool() const noexcept;

		void operator()();

		Task& operator=(Task&& other) noexcept;

		Task& operator=(const Task&) = delete;

	private:

		void destroy() noexcept;

		void move_from(Task&& other) noexcept;

		static constexpr size_t StorageSize  = 128;
		static constexpr size_t StorageAlign = alignof(std::max_align_t);
		using DestroyFn			     = void (*)(void*) noexcept;
		using InvokeFn			     = void (*)(void*);
		using MoveFn			     = void (*)(void*, void*) noexcept;
		using CopyFn			     = void (*)(void*, const void*);

		alignas(StorageAlign) std::byte m_storage[StorageSize];

		DestroyFn m_destroy = nullptr;
		InvokeFn  m_invoke  = nullptr;
		MoveFn	  m_move    = nullptr;
		CopyFn	  m_copy    = nullptr;
	};

	//

	template <typename F, typename IsNotTask> Task::Task(F&& func) noexcept {
		using FnT = std::decay_t<F>;

		// This gives a clear error message, if you have a Task as FnT then
		// the inline storage fails with the size, and you need to call std::move on the
		// task object.
		static_assert(!std::is_same_v<FnT, Task>,
			      "Error: Cannot construct a carbon::Task using another carbon::Task as the callable."
			      "Use std::move() for transfer, or a lambda to wrap the execution.");

		static_assert(sizeof(FnT) <= StorageSize, "Callable too large for Task inline storage!");
		static_assert(alignof(FnT) <= StorageAlign);

		// The storage is aligned to StorageAlign (std::max_align_t) so this does not have
		// alignment issues, regardless of FnT type.
		new(&m_storage) FnT(std::forward<F>(func));

		m_destroy = [](void* p) noexcept { reinterpret_cast<FnT*>(p)->~FnT(); };
		m_invoke  = [](void* p) { (*reinterpret_cast<FnT*>(p))(); };
		m_move = [](void* dst, void* src) noexcept { new(dst) FnT(std::move(*reinterpret_cast<FnT*>(src))); };

		m_copy = std::is_copy_constructible_v<FnT>
				 ? [](void* dst, const void* src) { new(dst) FnT(*reinterpret_cast<const FnT*>(src)); }
				 : nullptr;
	}

	Task::Task(const Task& other) {
		// An empty Task has all its function pointers set to nullptr.
		if(!other.m_invoke) {
			return;
		}

		// Runtime check: Ensure we are not copying a non-copyable object.
		assert(other.m_copy &&
		       "Task was constructed from a non-copyable callable, but copy constructor was invoked!");

		// Copy the vtable
		m_invoke  = other.m_invoke;
		m_move	  = other.m_move;
		m_destroy = other.m_destroy;
		m_copy	  = other.m_copy;

		// Use the original callable's copy function to construct the object in our storage.
		other.m_copy(&m_storage, &other.m_storage);
	}

	Task::Task(Task&& other) { move_from(std::move(other)); }

	Task::~Task() { destroy(); }

	Task::operator bool() const noexcept { return m_invoke; }

	void Task::operator()() {

		assert(m_invoke);
		m_invoke(&m_storage);
	}

	Task& Task::operator=(Task&& other) noexcept {

		if(this != &other) {
			destroy();
			move_from(std::move(other));
		}

		return *this;
	}

	void Task::destroy() noexcept {

		if(m_destroy) {
			m_destroy(&m_storage);
		}
	}

	void Task::move_from(Task&& other) noexcept {

		// If "other" is empty, we leave "this" in an empty state.
		if(!other.m_invoke) {
			return;
		}

		// Transfer vtable.
		m_destroy = other.m_destroy;
		m_invoke  = other.m_invoke;
		m_move	  = other.m_move;
		m_copy	  = other.m_copy;
		m_move(&m_storage, &other.m_storage);

		// Reset other to empty.
		other.m_destroy = nullptr;
		other.m_invoke	= nullptr;
		other.m_move	= nullptr;
		other.m_copy	= nullptr;
	}

} // namespace opus3d