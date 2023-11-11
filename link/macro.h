#ifndef __LINKS_MACRO_H__
#define __LINKS_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#define LINK_ASSERT(x) \
    if (!(x)) { \
        LINK_LOG_ERROR(LINK_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << links::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define LINK_ASSERT2(x, w) \
    if (!(x)) { \
        LINK_LOG_ERROR(LINK_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << links::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
#endif
