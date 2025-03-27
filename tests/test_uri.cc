#include "../linko/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    linko::Uri::ptr uri = linko::Uri::Create("http://www.sylar.top/test/uri?id=100&name=sylar#frg");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
