#ifndef __LINKS_LOG_H__
#define __LINKS_LOG_H__

#include <cstdarg>
#include <map>
#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <vector>
#include <fstream>
#include "util.h"
#include "singleton.h"

#define LINK_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level) \
        links::LogEventWrap(links::LogEvent::ptr(new links::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, links::GetThreadId(), links::GetFiberId(), \
                        time(0), "TestName"))).getSS()

#define LINK_LOG_DEBUG(logger) LINK_LOG_LEVEL(logger, links::LogLevel::DEBUG)
#define LINK_LOG_INFO(logger) LINK_LOG_LEVEL(logger, links::LogLevel::INFO)
#define LINK_LOG_WARN(logger) LINK_LOG_LEVEL(logger, links::LogLevel::WARN)
#define LINK_LOG_ERROR(logger) LINK_LOG_LEVEL(logger, links::LogLevel::ERROR)
#define LINK_LOG_FATAL(logger) LINK_LOG_LEVEL(logger, links::LogLevel::FATAL)

#define LINK_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level) \
        links::LogEventWrap(links::LogEvent::ptr(new links::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, links::GetThreadId(), links::GetFiberId(), \
                        time(0), "TestName"))).getEvent()->format(fmt, __VA_ARGS__)

#define LINK_LOG_FMT_DEBUG(logger, fmt, ...) LINK_LOG_FMT_LEVEL(logger, links::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LINK_LOG_FMT_INFO(logger, fmt, ...) LINK_LOG_FMT_LEVEL(logger, links::LogLevel::INFO, fmt, __VA_ARGS__)
#define LINK_LOG_FMT_WARN(logger, fmt, ...) LINK_LOG_FMT_LEVEL(logger, links::LogLevel::WARN, fmt, __VA_ARGS__)
#define LINK_LOG_FMT_ERROR(logger, fmt, ...) LINK_LOG_FMT_LEVEL(logger, links::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LINK_LOG_FMT_FATAL(logger, fmt, ...) LINK_LOG_FMT_LEVEL(logger, links::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LINK_LOG_ROOT() links::LoggerMgr::GetInstance()->getRoot()
#define LINK_LOG_NAME(name) links::LoggerMgr::GetInstance()->getLogger(name)

namespace links {

class Logger;
class LoggerManager;

class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char * ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};


class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            , const char* file, int32_t line, uint32_t elapse
            , uint32_t thread_id, uint32_t fiber_id, uint64_t time
            , const std::string& thread_name);

    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    const std::string& getThreadName() const { return m_threadName; }
    std::string getContent() const { return m_ss.str(); }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }
    std::stringstream& getSS() { return m_ss; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
private:
    //file name
    const char* m_file{nullptr};
    //line num
    int32_t m_line{0};
    //cost time
    uint32_t m_elapse{0};
    //thread id
    uint32_t m_threadId{0};
    //fiber id
    uint32_t m_fiberId{0};
    //timing
    uint64_t m_time{0};
    //thread name
    const std::string m_threadName;
    //logger content stream
    std::stringstream m_ss;
    //Logger
    std::shared_ptr<Logger> m_logger;
    //log level
    LogLevel::Level m_level;
};


class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event; }
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};


class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();

    bool isError() const { return m_error; }

    const std::string getPattern() const { return m_pattern; }
private:
    //log format
    std::string m_pattern;
    //parsed log format
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};


class LogAppender {
    friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;

    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger,  LogLevel::Level level, LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    void setLevel(LogLevel::Level level) { m_level = level; }
    LogLevel::Level getLevel() const { return m_level; }
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    bool m_hasFormatter = false;
    LogFormatter::ptr m_formatter;
};


class Logger 
    : public std::enable_shared_from_this<Logger>
{
    friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val;}
    const std::string& getName() const {return m_name;}
    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    
    LogFormatter::ptr getFormatter();

    std::string toYamlString();

private:
    std::string m_name;
    LogLevel::Level m_level;

    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};


class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    bool reopen();
    std::string toYamlString() override;
private:
    std::string m_filename;
    std::ofstream m_filestream;
};


class LoggerManager {
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();

    Logger::ptr getRoot() const { return m_root; }

    std::string toYamlString();

private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef links::Singleton<LoggerManager> LoggerMgr;

}

#endif
