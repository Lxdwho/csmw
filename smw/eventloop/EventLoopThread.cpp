/**
 * @brief: EventLoopThread 实现
 *
 * SubLoop 栈上创建，join 后自动析构，生命周期清晰
 * PollerType 传递给 SubLoop 构造函数
 */

#include "EventLoopThread.h"

namespace smw       {
namespace eventloop {

EventLoopThread::EventLoopThread(ThreadInitCallback cb, const std::string& name,
                                 EventLoop::PollerType type)
    : pollerType_(type)
    , loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , callback_(std::move(cb))
{
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

SubLoop* EventLoopThread::startLoop() {
    thread_.start();

    SubLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    SubLoop loop(pollerType_);

    if (callback_) {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();  // 阻塞，直到 quit()

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
    // loop 栈上析构 → ~SubLoop() → 清理 ownedSlots_
}

} // namespace eventloop
} // namespace smw
