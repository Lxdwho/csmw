/**
 * @brief: PollPoller 实现
 *
 * poll() 后直接遍历 pollfds_ 数组，找 revents != 0 的 fd
 * O(n) 一次遍历，不需要二次查找
 */

#include "PollPoller.h"
#include "../logger/log.h"
#include <cstring>
#include <cerrno>

namespace smw       {
namespace eventloop {

PollPoller::PollPoller() = default;
PollPoller::~PollPoller() = default;

int PollPoller::poll(int timeoutMs) {
    int ret = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    if (ret < 0 && errno != EINTR) {
        log_error("[PollPoller] poll 失败: %s", strerror(errno));
    }
    return ret;
}

void PollPoller::forEachReadyFd(std::function<bool(int fd, short revents)> cb) {
    for (auto& pfd : pollfds_) {
        if (pfd.revents == 0) continue;
        if (!cb(pfd.fd, pfd.revents)) return;
    }
}

void PollPoller::addFd(int fd, short events) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;

    auto it = fdIndexMap_.find(fd);
    if (it != fdIndexMap_.end()) {
        pollfds_[it->second] = pfd;
    } else {
        fdIndexMap_[fd] = pollfds_.size();
        pollfds_.push_back(pfd);
    }
}

void PollPoller::removeFd(int fd) {
    auto it = fdIndexMap_.find(fd);
    if (it == fdIndexMap_.end()) return;

    size_t idx = it->second;
    if (idx != pollfds_.size() - 1) {
        pollfds_[idx] = pollfds_.back();
        fdIndexMap_[pollfds_[idx].fd] = idx;
    }
    pollfds_.pop_back();
    fdIndexMap_.erase(it);
}

void PollPoller::updateFd(int fd, short events) {
    auto it = fdIndexMap_.find(fd);
    if (it == fdIndexMap_.end()) return;
    pollfds_[it->second].events = events;
}

} // namespace eventloop
} // namespace smw
