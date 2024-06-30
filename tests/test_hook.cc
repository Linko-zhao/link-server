#include "../linko/hook.h"
#include "../linko/log.h"
#include "../linko/iomanager.h"

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test_sleep() {
    linko::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        LINKO_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        LINKO_LOG_INFO(g_logger) << "sleep 3";
    });

    LINKO_LOG_INFO(g_logger) << "test_sleep";
}

int main(int argc, char** argv) {
    test_sleep();
    return 0;
}
