#ifndef LOG_H
#define LOG_H

#include <string>
#include <memory>

namespace slyar {
class LogEvent{
public:
    LogEvent();
private:
    //file name
    const char* m_file{nullptr};
    //line num
    int32_t m_line{0};
    //
    uint32_t m_elapse{0};
    //thread id
    uint32_t m_threadId{0};
    //fiber id
    uint32_t m_fiberId{0};
    //
    uint64_t m_time{0};
    //
    std::string m_threadName;
    //
    std::stringstream m_ss;
    //
    std::shared_ptr<Logger> m_logger;
    //
    
};

class Logger {
public:
    Logger();
private:
};

class LogAppender {
public:
    virtual ~LogAppender() {}
private:

};

class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
private:
};

}

#endif
