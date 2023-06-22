#ifndef __LINKS_UTIL_H__
#define __LINKS_UTIL_H__

#include <cstdint>
#include <unistd.h>
#include <sys/types.h>

namespace links {

pid_t GetThreadId();

uint32_t GetFiberId();

}

#endif
