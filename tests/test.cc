#include <iostream>
#include "../link/log.h"

int main (int argc, char** argv) {
    links::Logger::ptr logger(new links::Logger);
    logger->addAppender(links::LogAppender::ptr(new links::StdoutLogAppender));
    links::LogEvent::ptr event(new links::LogEvent(logger, 
    return 0;
}
