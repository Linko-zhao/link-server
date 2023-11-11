#ifndef __LINKS_UTIL_H__
#define __LINKS_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace links {

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size, int skip);
std::string BacktraceToString(int size, int skip = 1, const std::string& prefix = "");
}

#endif
