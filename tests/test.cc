#include <ctime>
#include <iostream>
#include "../linko/log.h"
#include "../linko/util.h"

int main (int argc, char** argv) {
    linko::Logger::ptr logger(new linko::Logger);
    logger->addAppender(linko::LogAppender::ptr(new linko::StdoutLogAppender));
    linko::FileLogAppender::ptr file_appender(new linko::FileLogAppender("./log.txt"));
    linko::LogFormatter::ptr fmt(new linko::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(linko::LogLevel::DEBUG);
    logger->addAppender(file_appender);
    //linko::LogEvent::ptr event(new linko::LogEvent(logger, linko::LogLevel::DEBUG, __FILE__, __LINE__, 0, linko::GetThreadId(),linko::GetFiberId(), time(0), "test"));
    //logger->log(linko::LogLevel::DEBUG, event);
    LINKO_LOG_DEBUG(logger) << "test macro";
    LINKO_LOG_FMT_DEBUG(logger, "test format debug %s", "dddd");

    auto l = linko::LoggerMgr::GetInstance()->getLogger("xxx");
    l->addAppender(linko::LogAppender::ptr(new linko::StdoutLogAppender));
    LINKO_LOG_DEBUG(l) << "ttt";
    return 0;
}
