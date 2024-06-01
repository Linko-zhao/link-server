#include "../linko/links.h"

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

int count = 0;
linko::RWMutex s_mutex;

void fun1() {
    LINKO_LOG_INFO(g_logger) << "name:" << linko::Thread::GetName()
                            << " this.name:" << linko::Thread::GetThis()->getName()
                            << " id:" << linko::GetThreadId()
                            << " this.id:" << linko::Thread::GetThis()->getId();

    for (int i = 0; i < 1000000; ++i) {
        linko::RWMutex::WriteLock lock(s_mutex);
        count++;
    }
}

void fun2() {
    while (true) {
        LINKO_LOG_INFO(g_logger) << "=================================================";
    }
}

void fun3() {
    while (true) {
        LINKO_LOG_INFO(g_logger) << "*************************************************";
    }
}

int main(int argc, char** argv) {
    LINKO_LOG_INFO(g_logger) << "thread test begin";

    YAML::Node root = YAML::LoadFile("/home/linko/Code/link-server/bin/conf/log2.yml");
    linko::Config::LoadFromYaml(root);

    std::vector<linko::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i) {
        linko::Thread::ptr thr(new linko::Thread(&fun2, "name_" + std::to_string(i * 2)));
        linko::Thread::ptr thr2(new linko::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    
    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }

    LINKO_LOG_INFO(g_logger) << "thread test end";

    LINKO_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}
