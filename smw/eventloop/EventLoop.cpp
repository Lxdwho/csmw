/**
 * @brief: EventLoop 基类实现
 *
 * 使用 Poller 抽象层进行 I/O 多路复用
 * 运行时通过 PollerType 参数选择 poll 或 epoll
 */

#include "EventLoop.h"
#include "PollPoller.h"
#include "EpollPoller.h"
#include "../logger/log.h"
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace smw       {
namespace eventloop {

static int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        log_error("[EventLoop] 创建 eventfd 失败");
        return -1;
    }
    return evtfd;
}

static std::unique_ptr<Poller> createPoller(EventLoop::PollerType type) {
    switch (type) {
        case EventLoop::PollerType::kEpoll:
            return std::make_unique<EpollPoller>(false);  // epoll 水平触发
        case EventLoop::PollerType::kEpollEdge:
            return std::make_unique<EpollPoller>(true);   // epoll 边沿触发
        case EventLoop::PollerType::kPoll:
        default:
            return std::make_unique<PollPoller>();         // poll 水平触发
    }
}

EventLoop::EventLoop(PollerType type)
    : pollerType_(type)
    , poller_(createPoller(type))
    , looping_(false)
    , quit_(false)
    , wakeupFd_(createEventfd())
    , threadId_(0)
{
    /* 注册 wakeup fd */
    if (wakeupFd_ >= 0) {
        poller_->addFd(wakeupFd_, POLLIN);
    }
}

EventLoop::~EventLoop() {
    if (wakeupFd_ >= 0) {
        ::close(wakeupFd_);
    }
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    threadId_ = static_cast<pid_t>(::syscall(SYS_gettid));

    while (!quit_) {
        // 1. 子类注册自己关注的 fd
        registerFds(poller_.get());

        // 2. Poller 等待事件（10ms 超时）
        int ret = poller_->poll(1000);

        // 3. 遍历就绪 fd，处理唤醒事件和子类事件
        if (ret > 0) {
            poller_->forEachReadyFd([this](int fd, short revents) -> bool {
                if (fd == wakeupFd_ && (revents & POLLIN)) {
                    handleWakeup();
                    return true;  // 继续遍历
                }
                return true;  // 子类在 handleEvents 中处理
            });
        }

        // 4. 子类处理各自的就绪事件
        handleEvents(poller_.get());

        // 5. 执行待处理任务
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    wakeup();
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    wakeup();
}

void EventLoop::wakeup() {
    if (wakeupFd_ >= 0) {
        uint64_t one = 1;
        ::write(wakeupFd_, &one, sizeof(one));
    }
}

void EventLoop::handleWakeup() {
    if (wakeupFd_ >= 0) {
        uint64_t one = 0;
        ::read(wakeupFd_, &one, sizeof(one));
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const auto& func : functors) {
        func();
    }
}

bool EventLoop::isInLoopThread() const {
    return threadId_ == static_cast<pid_t>(::syscall(SYS_gettid));
}

} // namespace eventloop
} // namespace smw
