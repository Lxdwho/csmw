#ifndef _SMW_THREAD_THREAD_H_
#define _SMW_THREAD_THREAD_H_

#include "../common/Noncopyable.h"
#include <functional>
#include <string>
#include <thread>
#include <memory>
#include <atomic>
#include <semaphore.h>

namespace smw       {
namespace thread    {

class Thread : common::Noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = "");
    ~Thread();

    void start();
    void join();
    bool started() const { return started_; }
    const std::string& name() const { return name_; }

private:
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    ThreadFunc func_;
    std::string name_;
    sem_t sem_;

    static std::atomic_int numCreated_;
};

} // namespace thread
} // namespace smw

#endif // _SMW_THREAD_THREAD_H_
