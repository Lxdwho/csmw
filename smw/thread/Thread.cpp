#include "Thread.h"
#include "CurrentThread.h"

namespace smw       {
namespace thread    {

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , func_(std::move(func))
    , name_(name)
{
    sem_init(&sem_, 0, 0);
    if (name_.empty()) {
        name_ = "Thread" + std::to_string(++numCreated_);
    }
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;
    thread_ = std::make_shared<std::thread>([this] {
        CurrentThread::cacheTid();
        sem_post(&sem_);
        func_();
    });
    sem_wait(&sem_);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

} // namespace thread
} // namespace smw
