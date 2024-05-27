#ifndef __LINKO_UTIL_H__
#define __LINKO_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace linko {

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 1, const std::string& prefix = "");

uint64_t GetCurrentMS();
uint64_t GetCurrentUS();
}

#endif
