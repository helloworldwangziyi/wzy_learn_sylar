#include "log.h"
#include "config.h"
#include <utility> // for std::pair
#include <functional>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
namespace sylar
{

    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;
            XX(FATAL);
            XX(ALERT);
            XX(CRIT);
            XX(ERROR);
            XX(WARN);
            XX(NOTICE);
            XX(INFO);
            XX(DEBUG);
            XX(NOTSET);
#undef XX
        default:
            return "NOTSET";
        }
        return "NOTSET";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(FATAL, fatal);
        XX(ALERT, alert);
        XX(CRIT, crit);
        XX(ERROR, error);
        XX(WARN, warn);
        XX(NOTICE, notice);
        XX(INFO, info);
        XX(DEBUG, debug);
#undef XX

        return LogLevel::NOTSET;
    }

    LogEvent::LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line, int64_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name)
        : m_logger_name(logger_name), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_thread_id(thread_id), m_fiber_id(fiber_id), m_time(time), m_thread_name(thread_name)
    {
    }

    void LogEvent::printf(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        vprintf(fmt, al);
        va_end(al);
    }

    void LogEvent::vprintf(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return event->getContent().size();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(event->getLevel());
        }
        size_t getLen(LogEvent::ptr event)
        {
            return strlen(LogLevel::ToString(event->getLevel()));
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return std::to_string(event->getElapse()).size();
        }
    };

    class LoggerNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        LoggerNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLoggerName();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return event->getLoggerName().size();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return std::to_string(event->getThreadId()).size();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return std::to_string(event->getFiberId()).size();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return event->getThreadName().size();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }
        size_t getLen(LogEvent::ptr event)
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            return strlen(buf);
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
        size_t getLen(LogEvent::ptr event)
        {
            return event->getFile().size();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
        size_t getLen(LogEvent::ptr event)
        {
            std::string line = std::to_string(event->getLine());
            return line.size();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem // 换行
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << std::endl;
        }
        size_t getLen(LogEvent::ptr event)
        {
            return 0;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str = "") : m_string(str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << m_string;
        }
        size_t getLen(LogEvent::ptr event)
        {
            return m_string.size();
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem // 制表符
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "\t";
        }
        size_t getLen(LogEvent::ptr event)
        {
            return 1;
        }
    };

    class PercentFormatItem : public LogFormatter::FormatItem // 百分号
    {
    public:
        PercentFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "%";
        }
        size_t getLen(LogEvent::ptr event)
        {
            return 1;
        }
    };

    class SinglespaceFormatItem : public LogFormatter::FormatItem // 单空格
    {
    public:
        SinglespaceFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << " ";
        }
        size_t getLen(LogEvent::ptr event)
        {
            return 1;
        }
    };

    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    /**
     * 简单的状态机判断，提取pattern中的常规字符和模式字符
     *
     * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
     *
     * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
     *
     * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
     *
     * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
     *
     * @see LogFormatter::LogFormatter
     */
    void LogFormatter::init()
    {
        // 按顺序存储解析出来的模板项
        // 每个pattern包括一个整数类型和一个字符串，类型为0表示pattern是常规字符，为1表示pattern是模板转义字符
        // 日期格式用下面的dateformat存储
        std::vector<std::pair<int, std::string>> patterns;
        // 临时存储常规字符串
        std::string tmp;
        // 日期格式字符串，默认把位于%d后面的大括号对里的内容作为日期格式，不校验格式是否合法
        std::string dateformat;
        // 解析是否出错
        bool error = false;

        // 是否正在解析常规字符，初始时为true
        bool parsing_string = true;

        size_t i = 0;
        while (i < m_pattern.size())
        {
            std::string c = std::string(1, m_pattern[i]);
            if (c == "%")
            {
                if (parsing_string)
                {
                    if (!tmp.empty())
                    {
                        patterns.push_back(std::make_pair(0, tmp));
                    }
                    tmp.clear();
                    parsing_string = false; // 解析正常文字遇到%，表示开始解析模板字符
                    i++;
                    continue;
                }
                else
                {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;
                    i++;
                    continue;
                }
            }
            else // 不是%
            {
                if (parsing_string)
                {
                    tmp += c;
                    i++;
                    continue;
                }
                else
                {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;

                    if (c != "d")
                    {
                        i++;
                        continue;
                    }
                    i++;
                    if (i < m_pattern.size() && m_pattern[i] != '{')
                    {
                        continue;
                    }
                    i++;
                    while (i < m_pattern.size() && m_pattern[i] != '}')
                    {
                        dateformat.push_back(m_pattern[i]);
                        i++;
                    }
                    if (m_pattern[i] != '}')
                    {
                        // %d后面的大括号没有闭合，直接报错
                        std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                        error = true;
                        break;
                    }
                    i++;
                    continue;
                }
            }
        }
        if (error)
        {
            m_error = true;
            return;
        }
        // 模板解析结束之后剩余的常规字符也要算进去
        if (!tmp.empty())
        {
            patterns.push_back(std::make_pair(0, tmp));
            tmp.clear();
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C) {#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }}
            XX(m, MessageFormatItem),     // m:消息
            XX(p, LevelFormatItem),       // p:日志级别
            XX(c, LoggerNameFormatItem),  // c:日志器名称
                                          //        XX(d, DateTimeFormatItem),          // d:日期时间
            XX(r, ElapseFormatItem),      // r:累计毫秒数
            XX(f, FilenameFormatItem),    // f:文件名
            XX(l, LineFormatItem),        // l:行号
            XX(t, ThreadIdFormatItem),    // t:编程号
            XX(F, FiberIdFormatItem),     // F:协程号
            XX(N, ThreadNameFormatItem),  // N:线程名称
            XX(%, PercentFormatItem),     // %:百分号
            XX(T, TabFormatItem),         // T:制表符
            XX(n, NewLineFormatItem),     // n:换行符
            XX(z, SinglespaceFormatItem), // z:单空格
#undef XX
        };

        for (auto &v : patterns)
        {
            if (v.first == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
            }
            else if (v.second == "d")
            {
                m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
            }
            else
            {
                auto it = s_format_items.find(v.second);
                if (it == s_format_items.end())
                {
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " << "unknown format item: " << v.second << std::endl;
                    error = true;
                    break;
                }
                else
                {
                    m_items.push_back(it->second(v.second));
                }
            }
        }
        if (error)
        {
            m_error = true;
            return;
        }
    }
    std::string LogFormatter::format(LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &os, LogEvent::ptr event)
    {
        std::streampos start_pos = os.tellp(); // 获取写入前的位置
        for (auto &i : m_items)
        {
            i->format(os, event);
        }
        if (&os == &std::cout)
        { // 当前是输出到控制台 不判断文件大小
            return os;
        }
        std::streampos end_pos = os.tellp();                // 获取写入后的位置
        std::streamsize written_size = end_pos - start_pos; // 计算写入的字符数
        if ((size_t)written_size > 1000)
        { // 当前文件大小超过最大限制 那么就提示错误信息 并返回写入之前的位置
            std::cout << "Written size: " << written_size << " characters" << std::endl;
            os.seekp(start_pos); // 回到写入前的位置
        }
        else
        { // 当前文件大小没有超过最大限制 那么就更新要写入的字符数
        }

        std::cout << "Written size: " << written_size << " characters" << std::endl;
        return os;
    }

    size_t LogFormatter::getLength(LogEvent::ptr event)
    {
        size_t len = 0;
        for (auto &i : m_items)
        {
            len += i->getLen(event);
        }
        return len;
    }

    LogAppender::LogAppender(LogFormatter::ptr default_formatter) : m_default_formatter(default_formatter)
    {
    }
    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
    }
    LogFormatter::ptr LogAppender::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter ? m_formatter : m_default_formatter;
    }
    StdoutLogAppender::StdoutLogAppender()
        : LogAppender(LogFormatter::ptr(new LogFormatter))
    {
    }

    void StdoutLogAppender::log(LogEvent::ptr event)
    {
        if (m_formatter)
        {
            m_formatter->format(std::cout, event);
        }
        else
        {
            m_default_formatter->format(std::cout, event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["pattern"] = m_formatter ? m_formatter->getPattern() : m_default_formatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &file)
        : LogAppender(LogFormatter::ptr(new LogFormatter))
    {
        m_filename = file;
        m_filename_tmp = m_filename;
        m_filename = rename();
        reopen();
        if (m_reopenError)
        {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
    }

    std::string FileLogAppender::rename() // 重命名文件为文件加上日期和后缀
    {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

        // 创建一个时间输入输出流
        std::ostringstream os;
        // 定义时间格式
        std::string format = "%Y-%m-%d";
        os.imbue(std::locale(std::cout.getloc(), new boost::posix_time::time_facet(format.c_str())));
        // 将时间写入流
        os << now;

        std::string new_filename = m_filename_tmp;

        new_filename += "_" + os.str();

        new_filename += ".txt";

        // 重命名文件
        return new_filename;
    }

    /**
     * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
     */
    void FileLogAppender::log(LogEvent::ptr event)
    {
        uint64_t now = event->getTime();
        if (now >= (m_lastTime + 3))
        {
            m_filename = rename();
            reopen();
            if (m_reopenError)
            {
                std::cout << "reopen file " << m_filename << " error" << std::endl;
            }
            m_lastTime = now;
        }

        if (m_reopenError)
        {
            return;
        }
        MutexType::Lock lock(m_mutex);
        if (m_formatter)
        {
            size_t log_length = m_formatter->getLength(event);
            std::cout << "log_length: " << log_length << std::endl;
            if (!m_formatter->format(m_filestream, event))
            {
                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        }
        else
        {
            size_t log_length = m_default_formatter->getLength(event);
            std::cout << "log_length: " << log_length << std::endl;
            if (!m_default_formatter->format(m_filestream, event))
            {

                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        }
    }
    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios::app);
        m_reopenError = !m_filestream;
        return !m_reopenError;
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        node["pattern"] = m_formatter ? m_formatter->getPattern() : m_default_formatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    Logger::Logger(const std::string &name)
        : m_name(name), m_level(LogLevel::INFO), m_create_time(GetElapsedMS())
    {
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }
    /**
     * 调用Logger的所有appenders将日志写一遍，
     * Logger至少要有一个appender，否则没有输出
     */
    void Logger::log(LogEvent::ptr event)
    {
        if (event->getLevel() <= m_level)
        {
            for (auto &i : m_appenders)
            {
                i->log(event);
            }
        }
    }
    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
        : m_logger(logger), m_event(event)
    {
    }
    /**
     * @note LogEventWrap在析构时写日志
     */
    LogEventWrap::~LogEventWrap() {
        m_logger->log(m_event);
    }

    LoggerManager::LoggerManager() {
        m_root.reset(new Logger("root"));
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }

    /**
     * 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
     * 需要手动添加Appender
     */
    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        return logger;
    }
    /**
     * @todo 实现从配置文件加载日志配置
     */
    void LoggerManager::init()
    {
    }
    
    std::string LoggerManager::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for(auto& i : m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

}