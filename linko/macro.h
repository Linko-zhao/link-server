#ifndef __LINKO_MACRO_H__
#define __LINKO_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#define LINKO_ASSERT(x) \
    if (!(x)) { \
        LINKO_LOG_ERROR(LINKO_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << linko::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define LINKO_ASSERT2(x, w) \
    if (!(x)) { \
        LINKO_LOG_ERROR(LINKO_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << linko::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
#endif
