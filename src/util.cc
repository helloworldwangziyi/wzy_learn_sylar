/**
 * @file util.cpp
 * @brief util函数实现
 * @version 0.1
 * @date 2021-06-08
 */

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h> // for kill()
#include <sys/syscall.h>
#include <sys/stat.h>
#include <execinfo.h> // for backtrace()
#include <cxxabi.h>   // for abi::__cxa_demangle()
#include <algorithm>  // for std::transform()
#include "util.h"
#include "log.h"

namespace sylar
{

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }
    uint64_t GetElapsedMS()
    {
        struct timespec ts = {0};
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
    std::string GetThreadName()
    {
        char thread_name[16] = {0};
        pthread_getname_np(pthread_self(), thread_name, 16);
        return std::string(thread_name);
    }

    void SetThreadName(const std::string &name)
    {
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
    }

    uint64_t GetCurrentMS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

    std::string ToUpper(const std::string &name)
    {
        std::string rt = name;
        std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
        return rt;
    }

    std::string ToLower(const std::string &name)
    {
        std::string rt = name;
        std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
        return rt;
    }
    std::string Time2Str(time_t ts, const std::string &format)
    {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }

    time_t Str2Time(const char *str, const char *format)
    {
        struct tm t;
        memset(&t, 0, sizeof(t));
        if (!strptime(str, format, &t))
        {
            return 0;
        }
        return mktime(&t);
    }

}