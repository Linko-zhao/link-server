#include "../link/links.h"

static links::Logger::ptr g_logger = LINK_LOG_ROOT();

void test_fiber() {
    LINK_LOG_INFO(g_logger) << "test in fiber";

    static int s_count = 5;
    sleep(1);
    if (--s_count >= 0) {
        links::Scheduler::GetThis()->schedule(&test_fiber, links::GetThreadId());
    }
}

int main(int argc, char** argv) {
    links::Scheduler sc(1, false, "test");
    sc.start();
    LINK_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    LINK_LOG_INFO(g_logger) << "over";
    return 0;
}
