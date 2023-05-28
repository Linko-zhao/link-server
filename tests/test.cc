#include <ctime>
#include <iostream>
#include "../link/log.h"
#include "../link/util.h"

int main (int argc, char** argv) {
    links::Logger::ptr logger(new links::Logger);
    logger->addAppender(links::LogAppender::ptr(new links::StdoutLogAppender));
    links::FileLogAppender::ptr file_appender(new links::FileLogAppender("./log.txt"));
    //links::LogEvent::ptr event(new links::LogEvent(logger, links::LogLevel::DEBUG, __FILE__, __LINE__, 0, links::GetThreadId(),links::GetFiberId(), time(0), "test"));
    //logger->log(links::LogLevel::DEBUG, event);
    LINK_LOG_DEBUG(logger) << "test macro";
    return 0;
}
