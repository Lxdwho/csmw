/**
 * @brief: EventLoopThreadPool —— SubLoop 线程池
 *
 * 管理 N 个工作线程，每个线程运行一个 SubLoop
 * getNextLoop() 轮询分配，用于传感器 I/O 负载均衡
 *
 * 成员声明顺序保证 loops_ 先于 threads_ 析构：
 *   threads_ 析构 → join → SubLoop 栈上析构（清理传感器）
 *   loops_ 析构   → 指针已无效，但不再使用
 */

#ifndef SMW_EVENTLOOOP_EVENTLOOPTHREADPOLL_H_
#define SMW_EVENTLOOOP_EVENTLOOPTHREADPOLL_H_

#include "../common/Noncopyable.h"
#include "EventLoop.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace smw       {
namespace eventloop {

class SubLoop;
class EventLoopThread;

class EventLoopThreadPool : common::Noncopyable {
public:
    using ThreadInitCallback = std::function<void(SubLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name = "",
                        EventLoop::PollerType type = EventLoop::PollerType::kPoll);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = nullptr);

    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoops() const;

    bool started() const { return started_; }

private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;

    // 注意声明顺序：loops_ 必须在 threads_ 之前声明
    // 这样析构时 threads_ 先销毁（join），loops_ 后销毁（指针已无效但不再使用）
    EventLoop::PollerType pollerType_;
    std::vector<EventLoop*> loops_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_EVENTLOOPTHREADPOLL_H_
