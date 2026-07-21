/**
 * @brief: SubLoop 实现 —— 工作线程事件循环 + 传感器 I/O
 *
 * SubLoop 独占持有 driver，管理完整生命周期：
 *   addSensorSlot: Init → Start → 注册 fd
 *   removeSensorSlot: Stop → Release → 移除
 *   析构: 遍历所有 slot 做 Stop → Release
 *
 * 所有操作都在 SubLoop 自己的线程中执行，无需锁
 */

#include "SubLoop.h"
#include "../logger/log.h"
#include <algorithm>

namespace smw       {
namespace eventloop {

SubLoop::SubLoop(PollerType type)
    : EventLoop(type)
{
}

SubLoop::~SubLoop() {
    for (auto& slot : ownedSlots_) {
        log_info("[SubLoop] 析构清理传感器: %s", slot->devnode.c_str());
        if (slot->driver) {
            slot->driver->Stop();
            slot->driver->Release();
        }
    }
    ownedSlots_.clear();
    fdToSlotMap_.clear();
}

// ==================== 传感器生命周期 ====================

void SubLoop::addSensorSlot(std::shared_ptr<SensorSlot> slot) {
    std::string devnode = slot->devnode;
    std::string driver_name = slot->driver_name;
    SensorBase* rawPtr = slot->driver.get();

    /* Init */
    if (rawPtr->Init() != 0) {
        log_error("[SubLoop] 驱动初始化失败: %s", devnode.c_str());
        return;
    }

    /* Start */
    if (rawPtr->Start() != 0) {
        log_error("[SubLoop] 驱动启动失败: %s", devnode.c_str());
        return;
    }

    log_info("[SubLoop] 传感器 %s 已接管 (驱动=%s, fd=%d, 当前传感器数: %zu)",
             devnode.c_str(), driver_name.c_str(), rawPtr->fd(),
             ownedSlots_.size() + 1);

    int fd = rawPtr->fd();
    size_t idx = ownedSlots_.size();
    ownedSlots_.push_back(std::move(slot));
    if (fd >= 0) {
        fdToSlotMap_[fd] = idx;
    }
}

void SubLoop::removeSensorSlot(const std::string& devnode) {
    auto it = std::find_if(ownedSlots_.begin(), ownedSlots_.end(),
                           [&](auto& s) { return s->devnode == devnode; });
    if (it == ownedSlots_.end()) {
        log_info("[SubLoop] 传感器 %s 不存在", devnode.c_str());
        return;
    }

    auto& slot = *it;
    log_info("[SubLoop] 移除传感器: %s", devnode.c_str());

    int fd = slot->driver->fd();
    slot->driver->Stop();
    slot->driver->Release();

    // 从 fdToSlotMap_ 移除
    if (fd >= 0) {
        fdToSlotMap_.erase(fd);
    }

    // 从 ownedSlots_ 移除（后面的元素索引需要更新）
    size_t idx = it - ownedSlots_.begin();
    ownedSlots_.erase(it);
    // 更新被影响的索引
    for (auto& pair : fdToSlotMap_) {
        if (pair.second > idx) {
            pair.second--;
        }
    }
}

std::vector<SubLoop::SensorInfo> SubLoop::getSensorInfo() const {
    std::vector<SensorInfo> result;
    result.reserve(ownedSlots_.size());
    for (auto& slot : ownedSlots_) {
        SensorInfo info;
        info.devnode = slot->devnode;
        info.driver_name = slot->driver_name;
        info.status = slot->driver->GetStatus();
        info.fd = slot->driver->fd();
        result.push_back(info);
    }
    return result;
}

// ==================== Poller 钩子 ====================

void SubLoop::registerFds(Poller* poller) {
    for (auto& slot : ownedSlots_) {
        int fd = slot->driver->fd();
        if (fd < 0) continue;

        // 只监听 POLLIN，不注册 POLLOUT（串口永远可写，注册会导致忙循环）
        // 请求-响应型传感器的写由 MainLoop timerfd 通过 queueInLoop 投递
        poller->addFd(fd, POLLIN);
    }
}

void SubLoop::handleEvents(Poller* poller) {
    poller->forEachReadyFd([this](int fd, short revents) -> bool {
        auto it = fdToSlotMap_.find(fd);
        if (it == fdToSlotMap_.end()) {
            return true;  // 不是传感器 fd，跳过
        }

        size_t idx = it->second;
        if (idx >= ownedSlots_.size()) return true;

        auto& driver = ownedSlots_[idx]->driver;

        if (revents & POLLIN) {
            driver->ReadData();    // 可读 → 收数据 / 收响应
        }
        return true;
    });
}

} // namespace eventloop
} // namespace smw
