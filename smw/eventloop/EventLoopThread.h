/**
 * @brief: EventLoopThread —— 在独立线程中运行一个 SubLoop
 *
 * startLoop() 启动线程并阻塞等待 SubLoop 创建完成，返回 SubLoop 指针
 * 析构时 quit + join，保证 SubLoop 在线程内完成生命周期
 * PollerType 传递给 SubLoop，统一 I/O 多路复用策略
 */

#ifndef SMW_EVENTLOOOP_EVENTLOOPTHREAD_H_
#define SMW_EVENTLOOOP_EVENTLOOPTHREAD_H_

#include "../common/Noncopyable.h"
#include "Thread.h"
#include "EventLoop.h"
#include "SubLoop.h"
#include <mutex>
#include <condition_variable>

namespace smw       {
namespace eventloop {

class EventLoopThread : common::Noncopyable {
public:
    using ThreadInitCallback = std::function<void(SubLoop*)>;

    EventLoopThread(ThreadInitCallback cb = nullptr,
                    const std::string& name = "",
                    EventLoop::PollerType type = EventLoop::PollerType::kPoll);
    ~EventLoopThread();

    SubLoop* startLoop();

private:
    void threadFunc();

    EventLoop::PollerType pollerType_;
    SubLoop* loop_;
    bool exiting_;
    thread::Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_EVENTLOOPTHREAD_H_
