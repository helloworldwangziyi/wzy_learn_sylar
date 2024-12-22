/**
 * @file thread.h
 * @author wangziyi
 * @brief 线程模块
 */
#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include "mutex.h"
#include <string>

namespace sylar
{

class Thread : Noncopyable
{
public:
    using ptr = std::shared_ptr<Thread>;
    /**
     * @brief 构造函数
     * @param cb 线程回调函数
     * @param name 线程名称
     */
    Thread(std::function<void()> cb, const std::string &name);

    /**
     * @brief 析构函数
     */
    ~Thread();

    /**
     * @brief 获取线程ID
     * @return 线程ID
     */
    pid_t getId() const { return m_id; }

    /**
     * @brief 获取线程名称
     * @return 线程名称
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief 等待线程结束
     */
    void join();


    /**
     * @brief 获取当前线程指针
     * @return 当前线程指针
     */
    static Thread *GetThis();

    /**
     * @brief 获取当前线程名称
     * @return 当前线程名称
     */
    static const std::string &GetName();

    /**
     * @brief 设置当前线程名称
     * @param name 线程名称
     */
    static void SetName(const std::string &name);

private:

    /**
     * @brief 线程函数
     * @param arg 线程参数
     */
    static void *run(void *arg);


    /// 线程id
    pid_t m_id = -1;
    /// pthread_t
    pthread_t m_thread = 0;
    /// 线程回调函数
    std::function<void()> m_cb;
    /// 线程名称
    std::string m_name;
    /// 信号量
    Semaphore m_semaphore;
};

} // namespace sylar

#endif // __SYLAR_THREAD_H__