#include "../link/links.h"
#include <assert.h>

links::Logger::ptr g_logger = LINK_LOG_ROOT();

void test_assert() {
    LINK_LOG_INFO(g_logger) << links::BacktraceToString(10, 2, "     ");
    LINK_ASSERT2(0 == 1, "test assert2 xxx");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}
