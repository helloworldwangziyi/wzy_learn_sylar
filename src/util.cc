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
#include "fiber.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

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

    void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix) {
        if (access(path.c_str(), 0) != 0) {
            return;
        }
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr) {
            return;
        }
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)) != nullptr) {
            if (dp->d_type == DT_DIR) {
                if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                    continue;
                }
                ListAllFile(files, path + "/" + dp->d_name, subfix);
            } else if (dp->d_type == DT_REG) {
                std::string filename(dp->d_name);
                if (subfix.empty()) {
                    files.push_back(path + "/" + filename);
                } else {
                    if (filename.size() < subfix.size()) {
                        continue;
                    }
                    if (filename.substr(filename.length() - subfix.size()) == subfix) {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
        }
        closedir(dir);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i) {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    static std::string demangle(const char *str) {
        size_t size = 0;
        int status  = 0;
        std::string rt;
        rt.resize(256);
        if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
            char *v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
            if (v) {
                std::string result(v);
                free(v);
                return result;
            }
        }
        if (1 == sscanf(str, "%255s", &rt[0])) {
            return rt;
        }
        return str;
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip) {
        void **array = (void **)malloc((sizeof(void *) * size));
        size_t s     = ::backtrace(array, size);

        char **strings = backtrace_symbols(array, s);
        if (strings == NULL) {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
            return;
        }

        for (size_t i = skip; i < s; ++i) {
            bt.push_back(demangle(strings[i]));
        }

        free(strings);
        free(array);
    }


}