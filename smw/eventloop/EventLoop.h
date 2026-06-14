/**
 * @brief: EventLoop 基类 —— 公共事件循环框架
 *
 * 提供：事件循环骨架、任务队列、跨线程唤醒、线程判断
 * 子类通过 virtual 钩子注入各自的 I/O 关注点（MainLoop: udev, SubLoop: 传感器）
 *
 * I/O 多路复用通过 Poller 抽象层支持 poll 和 epoll，运行时可选：
 *   - kPoll:        poll 水平触发（默认）
 *   - kEpoll:       epoll 水平触发
 *   - kEpollEdge:   epoll 边沿触发
 */

#ifndef SMW_EVENTLOOOP_EVENTLOOP_H_
#define SMW_EVENTLOOOP_EVENTLOOP_H_

#include "../common/Noncopyable.h"
#include "Poller.h"
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>

namespace smw       {
namespace eventloop {

class EventLoop : common::Noncopyable {
public:
    using Functor = std::function<void()>;

    /* I/O 多路复用类型 */
    enum class PollerType {
        kPoll,        // poll 水平触发
        kEpoll,       // epoll 水平触发
        kEpollEdge,   // epoll 边沿触发
    };

    explicit EventLoop(PollerType type = PollerType::kPoll);
    virtual ~EventLoop();

    /* 事件循环 */
    void loop();
    void quit();

    /* 任务队列 */
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    void wakeup();

    /* 线程判断 */
    bool isInLoopThread() const;

    /* 获取当前 Poller 类型 */
    PollerType getPollerType() const { return pollerType_; }

protected:
    /**
     * @brief: 获取 Poller 指针（子类用于注册持久 fd）
     */
    Poller* getPoller() { return poller_.get(); }

    /**
     * @brief: 子类钩子 —— 注册动态 fd 到 Poller
     * 在每次循环开始时调用，用于注册会变化的 fd（如传感器 fd）
     * 持久 fd（如 wakeupFd、udevFd）应在创建时注册一次
     */
    virtual void registerFds(Poller* /*poller*/) {}

    /**
     * @brief: 子类钩子 —— 处理就绪的 fd
     * 遍历 poller 的就绪事件，子类根据 fd 做相应处理
     */
    virtual void handleEvents(Poller* /*poller*/) {}

private:
    void handleWakeup();
    void doPendingFunctors();

    PollerType pollerType_;
    std::unique_ptr<Poller> poller_;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;

    int wakeupFd_;
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;

    pid_t threadId_;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_EVENTLOOP_H_
