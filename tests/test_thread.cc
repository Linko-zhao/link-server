#include "../link/links.h"

links::Logger::ptr g_logger = LINK_LOG_ROOT();

int count = 0;
links::RWMutex s_mutex;

void fun1() {
    LINK_LOG_INFO(g_logger) << "name:" << links::Thread::GetName()
                            << " this.name:" << links::Thread::GetThis()->getName()
                            << " id:" << links::GetThreadId()
                            << " this.id:" << links::Thread::GetThis()->getId();

    for (int i = 0; i < 1000000; ++i) {
        links::RWMutex::WriteLock lock(s_mutex);
        count++;
    }
}

void fun2() {
    while (true) {
        LINK_LOG_INFO(g_logger) << "=================================================";
    }
}

void fun3() {
    while (true) {
        LINK_LOG_INFO(g_logger) << "*************************************************";
    }
}

int main(int argc, char** argv) {
    LINK_LOG_INFO(g_logger) << "thread test begin";

    YAML::Node root = YAML::LoadFile("/home/links/Code/link-server/bin/conf/log2.yml");
    links::Config::LoadFromYaml(root);

    std::vector<links::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i) {
        links::Thread::ptr thr(new links::Thread(&fun2, "name_" + std::to_string(i * 2)));
        links::Thread::ptr thr2(new links::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    
    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }

    LINK_LOG_INFO(g_logger) << "thread test end";

    LINK_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}
