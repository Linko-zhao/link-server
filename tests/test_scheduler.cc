#include "../linko/links.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test_fiber() {
    LINKO_LOG_INFO(g_logger) << "test in fiber";

    static int s_count = 5;
    sleep(1);
    if (--s_count >= 0) {
        linko::Scheduler::GetThis()->schedule(&test_fiber, linko::GetThreadId());
    }
}

int main(int argc, char** argv) {
    linko::Scheduler sc(1, false, "test");
    sc.start();
    LINKO_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    LINKO_LOG_INFO(g_logger) << "over";
    return 0;
}
