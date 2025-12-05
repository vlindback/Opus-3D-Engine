
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file boost-license_1.0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// File modified for inclusion in the Opus Game Engine.
// All modifications are under the MIT License.
// See LICENSE

#pragma once

#include <cstddef>

namespace boost
{
	namespace context
	{
		typedef void* fcontext_t;

		struct transfer_t
		{
			fcontext_t fctx;
			void*	   data;
		};

		transfer_t jump_fcontext(fcontext_t const to, void* vp);
		fcontext_t make_fcontext(void* sp, size_t size, void (*fn)(transfer_t));
		transfer_t ontop_fcontext(fcontext_t const to, void* vp, transfer_t (*fn)(transfer_t));

	} // namespace context
} // namespace boost