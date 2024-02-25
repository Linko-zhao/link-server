#include "../link/links.h"

links::Logger::ptr g_logger = LINK_LOG_ROOT();

void run_in_fiber() {
    LINK_LOG_INFO(g_logger) << "run_in_fiber begin";
    links::Fiber::YieldToHold();
    LINK_LOG_INFO(g_logger) << "run_in_fiber end";
    links::Fiber::YieldToHold();
}

void test_fiber() {
    LINK_LOG_INFO(g_logger) << "main begin -1";
    {
        links::Fiber::GetThis();
        LINK_LOG_INFO(g_logger) << "main begin";
        links::Fiber::ptr fiber(new links::Fiber(run_in_fiber));
        //fiber->swapIn();
        fiber->call();
        LINK_LOG_INFO(g_logger) << "main after swapIn";
        //fiber->swapIn();
        fiber->call();
        LINK_LOG_INFO(g_logger) << "main after end";
        //fiber->swapIn();
        fiber->call();
    }
    LINK_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    links::Thread::SetName("main");

    test_fiber();
    //std::vector<links::Thread::ptr> thrs;
    //for (int i = 0; i < 3; ++i) {
    //    thrs.push_back(links::Thread::ptr(
    //                new links::Thread(&test_fiber, "name_" + std::to_string(i))
    //                )); 
    //}

    //for (auto i : thrs) {
    //    i->join();
    //}
    return 0;
}
