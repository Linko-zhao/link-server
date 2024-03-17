#include "../link/links.h"
#include "../link/iomanager.h"

links::Logger::ptr g_logger = LINK_LOG_ROOT();

void test_fiber() {
    LINK_LOG_INFO(g_logger) << "test_fiber";
}

void test1() {
    links::IOManager iom;
    iom.schedule(&test_fiber);
}

int main(int argc, char** argv) {
    test1();
    return 0;
}
