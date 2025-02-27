#ifndef __LINKO_MACRO_H__
#define __LINKO_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
#   define  LINKO_LIKELY(x)     __builtin_expect(!!(x), 1)
#   define  LINKO_UNLIKELY(x)   __builtin_expect(!!(x), 0)
#else
#   define  LINKO_LIKELY(x)     (x)
#   define  LINKO_UNLIKELY(x)   (x)
#endif

#define LINKO_ASSERT(x) \
    if (LINKO_UNLIKELY(!(x))) { \
        LINKO_LOG_ERROR(LINKO_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << linko::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define LINKO_ASSERT2(x, w) \
    if (LINKO_UNLIKELY(!(x))) { \
        LINKO_LOG_ERROR(LINKO_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << linko::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
#endif
