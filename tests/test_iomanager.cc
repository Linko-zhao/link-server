#include "../linko/links.h"
#include "../linko/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

int sock = 0;

void test_fiber() {
    LINKO_LOG_INFO(g_logger) << "test_fiber";
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.2.172.185", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {

    } else if (errno == EINPROGRESS) {
        LINKO_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        linko::IOManager::GetThis()->addEvent(sock, linko::IOManager::READ, [](){
            LINKO_LOG_INFO(g_logger) << "read callback";
        });
        linko::IOManager::GetThis()->addEvent(sock, linko::IOManager::WRITE, [](){
            LINKO_LOG_INFO(g_logger) << "write callback";
            linko::IOManager::GetThis()->cancelEvent(sock, linko::IOManager::READ);
            close(sock);
        });
    }else {
        LINKO_LOG_INFO(g_logger) << "else " << errno << " " <<strerror(errno);
    }

}

void test1() {
    linko::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

linko::Timer::ptr s_timer;
void test_timer() {
    linko::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        LINKO_LOG_INFO(g_logger) << "hello timer i=" << i;
        if (++i == 3) {
            //s_timer->reset(2000, true);
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    //test1();
    test_timer();
    return 0;
}
