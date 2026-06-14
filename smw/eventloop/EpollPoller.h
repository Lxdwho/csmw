/**
 * @brief: epoll 实现
 *
 * epoll_wait 返回的就是就绪 fd 列表，直接遍历即可
 * 支持水平触发和边沿触发
 */

#ifndef SMW_EVENTLOOOP_EPOLLPOLLER_H_
#define SMW_EVENTLOOOP_EPOLLPOLLER_H_

#include "Poller.h"
#include "../common/Noncopyable.h"
#include <vector>
#include <unordered_set>
#include <sys/epoll.h>

namespace smw       {
namespace eventloop {

class EpollPoller : public Poller {
public:
    explicit EpollPoller(bool edgeTrigger = false);
    ~EpollPoller() override;

    int poll(int timeoutMs) override;
    void forEachReadyFd(std::function<bool(int fd, short revents)> cb) override;
    void addFd(int fd, short events) override;
    void removeFd(int fd) override;
    void updateFd(int fd, short events) override;

private:
    int epfd_;
    bool edgeTrigger_;
    int readyCount_ = 0;
    std::vector<struct epoll_event> events_;
    std::unordered_set<int> registeredFds_;

    static short fromEpollEvents(uint32_t events);
    static uint32_t toEpollEvents(short events);
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_EPOLLPOLLER_H_
