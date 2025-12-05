#pragma once

#include "foundation/fibers/boost/fcontext.hpp"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory_resource>
#include <type_traits>

namespace opus3d::foundation
{
	class FiberContext
	{
	public:

		using FiberContextFunc = void (*)(void*);

		// Disable copying to prevent double-freeing stack or corrupting contexts
		FiberContext(const FiberContext&)	     = delete;
		FiberContext& operator=(const FiberContext&) = delete;

		FiberContext(void* stackBase, size_t stackSize, FiberContextFunc entry, void* userArg)
		    : m_func(entry), m_userArg(userArg) {
			// Compute top-of-stack pointer (stack grows downward on x64)
			uintptr_t sp = reinterpret_cast<uintptr_t>(stackBase) + stackSize;
			sp &= ~static_cast<uintptr_t>(0xF); // align down

			size_t alignedSize = sp - reinterpret_cast<uintptr_t>(stackBase);

			// Prepare the context.
			// We point to our own static entry stub.
			m_context = boost::context::make_fcontext(reinterpret_cast<void*>(sp), alignedSize,
								  &FiberContext::entry_stub);
		}

		// Do the initial handshake
		void start() {
			// First jump: parent -> fiber entry_stub
			// t.data inside entry_stub will be `this` (FiberContext*)
			auto t = boost::context::jump_fcontext(m_context, this);
			// When entry_stub yields back, t.fctx is fiber continuation
			m_context = t.fctx;
		}

		// Switch FROM the caller TO this fiber.
		boost::context::transfer_t resume() {
			auto t	  = boost::context::jump_fcontext(m_context, this);
			m_context = t.fctx;
			return t;
		}

		// Switch FROM this fiber BACK to parent
		void yield() {
			// Jump back to the parent.
			// We pass nullptr because we don't need to send data back for a simple yield.
			boost::context::transfer_t t = boost::context::jump_fcontext(m_parent, this);

			// Save where we resume from
			m_parent = t.fctx;
		}

		bool done() const noexcept { return m_finished; }

	private:

		boost::context::fcontext_t m_context{};
		boost::context::fcontext_t m_parent{};
		FiberContextFunc	   m_func     = {};
		void*			   m_userArg  = nullptr;
		bool			   m_finished = false;

		static void entry_stub(boost::context::transfer_t t) {
			// First time in this context
			FiberContext* self = static_cast<FiberContext*>(t.data);

			// t.fctx is the parent
			self->m_parent = t.fctx;

			// Handshake: yield back so parent can store our continuation
			t = boost::context::jump_fcontext(self->m_parent, self);

			// When we’re resumed for real:
			// From the fiber’s point of view, t.fctx is again the parent.
			self->m_parent = t.fctx;

			// Run user function
			if(self->m_func) {
				self->m_func(self->m_userArg);
			}

			self->m_finished = true;

			// Final jump back; never returns
			boost::context::jump_fcontext(self->m_parent, nullptr);
		}
	};

	template <typename Task> class Fiber
	{
	public:

		Fiber(void* stack, size_t size, Task task)
		    : m_task(static_cast<Task&&>(task)), m_ctx(stack, size, &Fiber::run_task, this) {
			m_ctx.start();
		}

		void resume() {
			if(!done()) {
				m_ctx.resume();
			}
		}

		void yield() { m_ctx.yield(); }
		bool done() const { return m_ctx.done(); }

	private:

		static void run_task(void* selfPtr) {
			auto* fiberPtr = static_cast<Fiber*>(selfPtr);
			fiberPtr->m_task(*fiberPtr);
		}

		Task	     m_task;
		FiberContext m_ctx;
	};
} // namespace opus3d::foundation