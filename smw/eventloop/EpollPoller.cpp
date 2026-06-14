/**
 * @brief: EpollPoller 实现
 *
 * epoll_wait 返回的就是就绪 fd 列表，直接遍历 events_ 数组
 * 不需要二次查找，O(就绪数)
 */

#include "EpollPoller.h"
#include "../logger/log.h"
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace smw       {
namespace eventloop {

EpollPoller::EpollPoller(bool edgeTrigger)
    : epfd_(::epoll_create1(EPOLL_CLOEXEC))
    , edgeTrigger_(edgeTrigger)
{
    if (epfd_ < 0) {
        log_error("[EpollPoller] epoll_create1 失败: %s", strerror(errno));
    }
    events_.resize(16);
}

EpollPoller::~EpollPoller() {
    if (epfd_ >= 0) {
        ::close(epfd_);
    }
}

int EpollPoller::poll(int timeoutMs) {
    readyCount_ = ::epoll_wait(epfd_, events_.data(), events_.size(), timeoutMs);
    if (readyCount_ < 0 && errno != EINTR) {
        log_error("[EpollPoller] epoll_wait 失败: %s", strerror(errno));
    }
    if (readyCount_ == static_cast<int>(events_.size())) {
        events_.resize(events_.size() * 2);
    }
    return readyCount_;
}

void EpollPoller::forEachReadyFd(std::function<bool(int fd, short revents)> cb) {
    for (int i = 0; i < readyCount_; ++i) {
        short revents = fromEpollEvents(events_[i].events);
        if (!cb(events_[i].data.fd, revents)) return;
    }
}

void EpollPoller::addFd(int fd, short events) {
    uint32_t epollEvents = toEpollEvents(events);
    if (edgeTrigger_) {
        epollEvents |= EPOLLET;
    }

    auto it = registeredFds_.find(fd);
    if (it != registeredFds_.end()) {
        struct epoll_event ev;
        ev.events = epollEvents;
        ev.data.fd = fd;
        ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
    } else {
        struct epoll_event ev;
        ev.events = epollEvents;
        ev.data.fd = fd;
        if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            log_error("[EpollPoller] epoll_ctl ADD 失败 (fd=%d): %s", fd, strerror(errno));
        } else {
            registeredFds_.insert(fd);
        }
    }
}

void EpollPoller::removeFd(int fd) {
    ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    registeredFds_.erase(fd);
}

void EpollPoller::updateFd(int fd, short events) {
    uint32_t epollEvents = toEpollEvents(events);
    if (edgeTrigger_) {
        epollEvents |= EPOLLET;
    }
    struct epoll_event ev;
    ev.events = epollEvents;
    ev.data.fd = fd;
    ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

uint32_t EpollPoller::toEpollEvents(short events) {
    uint32_t e = 0;
    if (events & POLLIN)  e |= EPOLLIN;
    if (events & POLLOUT) e |= EPOLLOUT;
    if (events & POLLERR) e |= EPOLLERR;
    if (events & POLLHUP) e |= EPOLLHUP;
    return e;
}

short EpollPoller::fromEpollEvents(uint32_t events) {
    short e = 0;
    if (events & EPOLLIN)  e |= POLLIN;
    if (events & EPOLLOUT) e |= POLLOUT;
    if (events & EPOLLERR) e |= POLLERR;
    if (events & EPOLLHUP) e |= POLLHUP;
    return e;
}

} // namespace eventloop
} // namespace smw
