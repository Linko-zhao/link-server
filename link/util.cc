#include "util.h"

namespace links {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return 1000;
}

}
