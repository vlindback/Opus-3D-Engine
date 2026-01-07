#pragma once

#include "panic.hpp"

// Use ASSERT and ASSERT_MSG to enforce invariants.

#define ASSERT(expr)                                                                                                                                           \
	do                                                                                                                                                     \
	{                                                                                                                                                      \
		if(!(expr))                                                                                                                                    \
		{                                                                                                                                              \
			::opus3d::foundation::panic("Assertion failed: " #expr, ::opus3d::foundation::SourceLocation());                                       \
		}                                                                                                                                              \
	} while(0)

#define ASSERT_MSG(expr, msg)                                                                                                                                  \
	do                                                                                                                                                     \
	{                                                                                                                                                      \
		if(!(expr))                                                                                                                                    \
		{                                                                                                                                              \
			::opus3d::foundation::panic("Assertion failed: " msg, ::opus3d::foundation::SourceLocation());                                         \
		}                                                                                                                                              \
	} while(0)

#if !defined(NDEBUG)
#define DEBUG_ASSERT(expr) ASSERT(expr)
#define DEBUG_ASSERT_MSG(expr, msg) ASSERT_MSG(expr, msg)
#else
#define DEBUG_ASSERT(expr) ((void)0)
#define DEBUG_ASSERT_MSG(expr, msg) ((void)0)
#endif
