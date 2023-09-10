#include "../link/links.h"

links::Logger::ptr g_logger = LINK_LOG_ROOT();

void fun1() {
    LINK_LOG_INFO(g_logger) << "name:" << links::Thread::GetName()
                            << " this.name:" << links::Thread::GetThis()->getName()
                            << " id:" << links::GetThreadId()
                            << " this.id:" << links::Thread::GetThis()->getId();
}

void fun2() {
    
}

int main(int argc, char** argv) {
    LINK_LOG_INFO(g_logger) << "thread test begin";

    std::vector<links::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        links::Thread::ptr thr(new links::Thread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    
    for (int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }

    LINK_LOG_INFO(g_logger) << "thread test end";

    return 0;
}
