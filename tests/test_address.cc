#include "../linko/address.h"
#include "../linko/log.h"

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test() {
    std::vector<linko::Address::ptr> addrs;

    bool v = linko::Address::Lookup(addrs, "www.baidu.com");
    if (!v) {
        LINKO_LOG_ERROR(g_logger) << "Lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        LINKO_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<linko::Address::ptr, uint32_t> > results;
    
    bool v = linko::Address::GetInterfaceAddress(results);
    if (!v) {
        LINKO_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return;
    }

    for (auto& i : results) {
        LINKO_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString()
            << " - " << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = linko::IPAddress::Create("www.baidu.com");
    auto addr = linko::IPAddress::Create("127.0.0.1");
    if (addr) {
        LINKO_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();
    //test_iface();
    //test();
    return 0;
}
