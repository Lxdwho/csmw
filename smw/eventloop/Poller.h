/**
 * @brief: I/O 多路复用抽象层
 *
 * 定义统一接口，支持 poll 和 epoll
 * 子类通过 forEachReadyFd 遍历就绪 fd，不需要 getEvent 二次查找
 */

#ifndef SMW_EVENTLOOOP_POLLER_H_
#define SMW_EVENTLOOOP_POLLER_H_

#include "../common/Noncopyable.h"
#include <functional>
#include <poll.h>

namespace smw       {
namespace eventloop {

class Poller : common::Noncopyable {
public:
    Poller() = default;
    virtual ~Poller() = default;

    /**
     * @brief: 等待事件
     * @param timeoutMs 超时时间（毫秒），-1 表示阻塞等待
     * @return 就绪的 fd 数量，-1 表示错误
     */
    virtual int poll(int timeoutMs) = 0;

    /**
     * @brief: 遍历所有就绪的 fd，对每个就绪 fd 调用回调
     * @param cb 回调函数，参数为 (fd, revents)，返回 true 继续遍历，false 停止
     *
     * poll 实现：遍历 pollfds_ 数组，找 revents != 0 的
     * epoll 实现：遍历 epoll_wait 返回的 events 数组
     */
    virtual void forEachReadyFd(std::function<bool(int fd, short revents)> cb) = 0;

    /**
     * @brief: 添加/更新 fd
     */
    virtual void addFd(int fd, short events) = 0;

    /**
     * @brief: 移除 fd
     */
    virtual void removeFd(int fd) = 0;

    /**
     * @brief: 修改 fd 关注的事件
     */
    virtual void updateFd(int fd, short events) = 0;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_POLLER_H_
