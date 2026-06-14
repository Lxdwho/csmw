/**
 * @brief: poll 实现 —— 水平触发
 *
 * 每次 poll() 调用将完整的 pollfd 数组拷贝到内核
 * forEachReadyFd 直接遍历 pollfds_ 数组，找 revents != 0 的 fd
 */

#ifndef SMW_EVENTLOOOP_POLLPOLLER_H_
#define SMW_EVENTLOOOP_POLLPOLLER_H_

#include "Poller.h"
#include "../common/Noncopyable.h"
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <poll.h>

namespace smw       {
namespace eventloop {

class PollPoller : public Poller {
public:
    PollPoller();
    ~PollPoller() override;

    int poll(int timeoutMs) override;
    void forEachReadyFd(std::function<bool(int fd, short revents)> cb) override;
    void addFd(int fd, short events) override;
    void removeFd(int fd) override;
    void updateFd(int fd, short events) override;

private:
    std::unordered_map<int, size_t> fdIndexMap_;
    std::vector<struct pollfd> pollfds_;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_POLLPOLLER_H_
