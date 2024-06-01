#include "../linko/links.h"

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void run_in_fiber() {
    LINKO_LOG_INFO(g_logger) << "run_in_fiber begin";
    linko::Fiber::YieldToHold();
    LINKO_LOG_INFO(g_logger) << "run_in_fiber end";
    linko::Fiber::YieldToHold();
}

void test_fiber() {
    LINKO_LOG_INFO(g_logger) << "main begin -1";
    {
        linko::Fiber::GetThis();
        LINKO_LOG_INFO(g_logger) << "main begin";
        linko::Fiber::ptr fiber(new linko::Fiber(run_in_fiber));
        //fiber->swapIn();
        fiber->call();
        LINKO_LOG_INFO(g_logger) << "main after swapIn";
        //fiber->swapIn();
        fiber->call();
        LINKO_LOG_INFO(g_logger) << "main after end";
        //fiber->swapIn();
        fiber->call();
    }
    LINKO_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    linko::Thread::SetName("main");

    test_fiber();
    //std::vector<linko::Thread::ptr> thrs;
    //for (int i = 0; i < 3; ++i) {
    //    thrs.push_back(linko::Thread::ptr(
    //                new linko::Thread(&test_fiber, "name_" + std::to_string(i))
    //                )); 
    //}

    //for (auto i : thrs) {
    //    i->join();
    //}
    return 0;
}
