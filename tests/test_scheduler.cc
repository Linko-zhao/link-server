#include "../link/links.h"

static links::Logger::ptr g_logger = LINK_LOG_ROOT();

int main(int argc, char** argv) {
    links::Scheduler sc;
    sc.start();
    sc.stop();
    return 0;
}
