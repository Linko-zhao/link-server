#include "util.h"

namespace links {

pid_t GetThreadId() {
    //return syscall(1001);
    return 1001;
}

uint32_t GetFiberId() {
    return 1000;
}

}
