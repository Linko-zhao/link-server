#include "log.h"
#include <boost/algorithm/string/predicate.hpp>

namespace sylar {

const char * LogLevel::ToString(LogLevel::level level) {
    switch (level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}


LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if (boost::iequals(str, #v)) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);   
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);
    
    return LogLevel::UNKNOW;
#undef XX
}


LogEventWrap::LogEventWrap(LogEvent::ptr e) 
    : m_event(e) {
}


LogEventWrap::~LogEventWrap() {
    
}


LogEvent::LogEvent(std::shared_ptr<LogEvent> logger, LogLevel::Level level
        , const char* file, int32_t line, uint32_t elapse
        , uint32_t thread_id, uint32_t fiber_id, uint64_t time
        , const std::string& thread_name)
    : m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(thread_id)
    , m_fiberId(fiber_id)
    , m_time(time)
    , m_threadName(thread_name)
    , m_logger(logger)
    , m_level(level) {
}

}
