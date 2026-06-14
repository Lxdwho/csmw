/**
 * @brief: EventLoopThreadPool 实现
 *
 * 线程池中每个线程创建 SubLoop（栈上），pool 只持有观察指针
 * PollerType 传递给每个 SubLoop，统一 I/O 多路复用策略
 */

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "SubLoop.h"

namespace smw       {
namespace eventloop {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name,
                                         EventLoop::PollerType type)
    : baseLoop_(baseLoop)
    , name_(name)
    , started_(false)
    , numThreads_(0)
    , next_(0)
    , pollerType_(type)
{
}

EventLoopThreadPool::~EventLoopThreadPool() {
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);

        auto t = std::make_unique<EventLoopThread>(cb, buf, pollerType_);
        loops_.push_back(t->startLoop());
        threads_.push_back(std::move(t));
    }

    if (numThreads_ == 0 && cb) {
        // 无线程模式：baseLoop_ 不是 SubLoop，这里不调回调
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    EventLoop* loop = baseLoop_;

    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= static_cast<int>(loops_.size())) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() const {
    if (loops_.empty()) {
        return {baseLoop_};
    }
    return loops_;
}

} // namespace eventloop
} // namespace smw
