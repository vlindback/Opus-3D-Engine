// SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// This code wraps the plain C asm symbols into properly
// namespaced and mangled C++ symbols that are safe to
// export from a library.

#include "fcontext.hpp"

#if (defined(i386) || defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i586__) ||                \
     defined(__i686__) || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) ||          \
     defined(__INTEL__) || defined(__IA32__) || defined(_M_IX86) || defined(_I86_)) &&                                 \
	defined(BOOST_WINDOWS)
#define BOOST_CONTEXT_CALLDECL __cdecl
#else
#define BOOST_CONTEXT_CALLDECL
#endif

extern "C" boost::context::transfer_t BOOST_CONTEXT_CALLDECL jump_fcontext(boost::context::fcontext_t const to,
									   void*			    vp);
extern "C" boost::context::fcontext_t BOOST_CONTEXT_CALLDECL make_fcontext(void* sp, size_t size,
									   void (*fn)(boost::context::transfer_t));

// based on an idea of Giovanni Derreta
extern "C" boost::context::transfer_t BOOST_CONTEXT_CALLDECL ontop_fcontext(
	boost::context::fcontext_t const to, void* vp, boost::context::transfer_t (*fn)(boost::context::transfer_t));

namespace boost
{
	namespace context
	{
		transfer_t jump_fcontext(fcontext_t const to, void* vp) { return ::jump_fcontext(to, vp); }

		fcontext_t make_fcontext(void* sp, size_t size, void (*fn)(transfer_t)) {
			return ::make_fcontext(sp, size, fn);
		}

		transfer_t ontop_fcontext(fcontext_t const to, void* vp, transfer_t (*fn)(transfer_t)) {
			return ::ontop_fcontext(to, vp, fn);
		}
	} // namespace context
} // namespace boost