#include "log.h"
#include<iostream>
using namespace std;

int main(){
    // test LogLevel
    cout << sylar::LogLevel::ToString(sylar::LogLevel::DEBUG) << endl;
    cout << sylar::LogLevel::FromString("DEBUG");
    // test LogEvent
    sylar::LogEvent log("test", sylar::LogLevel::DEBUG, "test.cc", 100, 0, 1, 2, time(0), "main");
    cout << log.getLevel() << endl;
    cout << log.getContent() << endl;
    cout << log.getFile() << endl;
    log.printf("wwangziyi %d", 1); 
    cout << log.getSS().str() << endl;
    // test LogFormatter
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d{%Y-%m-%d %H:%M:%S} [%rms]%z%t%z%N%z%F%z[%p]%z[%c]%z%f:%l%T%m%n"));
    sylar::LogEvent::ptr event = std::make_shared<sylar::LogEvent>("test", sylar::LogLevel::DEBUG, "test.cc", 100, 0, 1, 2, time(0), "main");
    event->getSS() << "hello sylar log";
    event->printf("wangziyi %d", 1);
    cout << event->getThreadName() << endl;
    cout << fmt->format(event) ;
    cout << fmt->isError() << endl;

    // test LogAppender 可以实现按照日期对日志进行分割 ，如果不想按照日期分割可以禁用rename方法
    sylar::LogAppender::ptr appender(new sylar::StdoutLogAppender);
    appender->log(event);
    cout << appender->toYamlString() << endl;
    sylar::FileLogAppender::ptr fileAppender(new sylar::FileLogAppender("../logfile/log1"));
    cout << fileAppender->toYamlString();
    fileAppender->log(event);
    fileAppender->log(event);
    fileAppender->log(event);
    // 打印当前文件的路径
    cout << __FILE__ << endl;

    // test Logger
    sylar::Logger::ptr logger(new sylar::Logger("test"));
    logger->addAppender(appender);
    logger->addAppender(fileAppender);
    logger->log(event);

    // test 宏定义
    SYLAR_LOG_INFO(logger) << "test macro";

    return 0;

}