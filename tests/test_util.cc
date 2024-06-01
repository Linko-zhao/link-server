#include "../linko/links.h"
#include <assert.h>

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test_assert() {
    LINKO_LOG_INFO(g_logger) << linko::BacktraceToString(10, 2, "     ");
    LINKO_ASSERT2(0 == 1, "test assert2 xxx");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}
