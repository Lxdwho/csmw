/**
 * @brief: MainLoop 实现 —— 主线程事件循环 + 传感器管理
 *
 * 合并自 SensorManger，职责内聚
 * MainLoop 只做匹配+创建+投递，不直接操作 driver
 * I/O 多路复用使用 poll()，模板方法模式注入 udev 关注点
 */

#include "MainLoop.h"
#include "SubLoop.h"
#include "../classFactory/ClassFactory.h"
#include "../logger/log.h"

#include <cstring>
#include <unistd.h>

namespace smw       {
namespace eventloop {

// ==================== 构造 / 析构 ====================

MainLoop::MainLoop(PollerType type)
    : EventLoop(type)
    , ioThreadPool_(this, "SubLoop", type)
    , udev_(nullptr)
    , udev_mon_(nullptr)
    , sacndevice_(false)
{
}

MainLoop::~MainLoop() {
    Stop();
}

// ==================== 配置 ====================

void MainLoop::setThreadNum(int num) {
    ioThreadPool_.setThreadNum(num);
}

// ==================== 驱动注册 ====================

void MainLoop::RegisterDriver(const DriverEntry& entry) {
    driver_registry_.push_back(entry);
    log_info("\n[Manager] 注册监听驱动: %s (子系统=%s, VID=%s, PID=%s)",
            entry.driver_name.c_str(),
            entry.subsystem.c_str(),
            entry.vendor_id.empty() ? "*" : entry.vendor_id.c_str(),
            entry.product_id.empty() ? "*" : entry.product_id.c_str());
}

// ==================== 启动 / 停止 ====================

int MainLoop::Start() {
    udev_ = ::udev_new();
    if (!udev_) {
        log_error("[Manager] udev_new 失败");
        return -1;
    }

    udev_mon_ = ::udev_monitor_new_from_netlink(udev_, "udev");
    if (!udev_mon_) {
        log_error("[Manager] udev_monitor 失败");
        ::udev_unref(udev_);
        udev_ = nullptr;
        return -1;
    }

    ::udev_monitor_filter_add_match_subsystem_devtype(udev_mon_, "usb", nullptr);
    ::udev_monitor_filter_add_match_subsystem_devtype(udev_mon_, "tty", nullptr);
    ::udev_monitor_filter_add_match_subsystem_devtype(udev_mon_, "net", nullptr);
    ::udev_monitor_enable_receiving(udev_mon_);

    /* udev fd 是持久的，注册一次即可 */
    int udev_fd = ::udev_monitor_get_fd(udev_mon_);
    if (udev_fd >= 0) {
        getPoller()->addFd(udev_fd, POLLIN);
    }

    ioThreadPool_.start();

    if (!ScanAllDevice()) {
        return -1;
    }

    log_info("\n[Manager] 热插拔监控已启动 (SubLoop线程数: %zu)",
             ioThreadPool_.getAllLoops().size());
    return 0;
}

void MainLoop::Stop() {
    slotMap_.clear();

    if (udev_mon_) {
        ::udev_monitor_unref(udev_mon_);
        udev_mon_ = nullptr;
    }
    if (udev_) {
        ::udev_unref(udev_);
        udev_ = nullptr;
    }

    quit();
    log_info("[Manager] 已停止");
}

// ==================== Poller 钩子 ====================

void MainLoop::handleEvents(Poller* poller) {
    if (!udev_mon_) return;

    int udev_fd = ::udev_monitor_get_fd(udev_mon_);

    poller->forEachReadyFd([this, udev_fd](int fd, short revents) -> bool {
        if (fd == udev_fd && (revents & POLLIN)) {
            while (handleUdevEvent()) {}
        }
        return true;
    });
}

// ==================== udev 热插拔 ====================

bool MainLoop::handleUdevEvent() {
    struct udev_device* dev = ::udev_monitor_receive_device(udev_mon_);
    if (!dev) return false;

    DeviceInfo info = ParseDeviceEvent(dev);
    ::udev_device_unref(dev);

    if (info.devnode.empty()) return true;

    if (info.action == "bind" || (info.subsystem == "tty" && info.action == "add")) {
        log_info("[Manager] 设备插入: %s (%s:%s)",
                 info.devnode.c_str(), info.vendor_id.c_str(),
                 info.product_id.c_str());

        if (FindDriver(info.subsystem, info.vendor_id, info.product_id)) {
            AddSensor(info.subsystem, info.vendor_id, info.product_id, info.devnode);
        } else {
            log_info("[Manager] 驱动监听未被注册 (子系统=%s, VID=%s, PID=%s)",
                     info.subsystem.c_str(), info.vendor_id.c_str(), info.product_id.c_str());
        }
    }
    else if (info.action == "remove") {
        log_info("\n[Manager] 设备移除: %s", info.devnode.c_str());
        RemoveSensor(info.devnode);
    }
    return true;
}

MainLoop::DeviceInfo MainLoop::ParseDeviceEvent(void* udev_dev) {
    auto* dev = static_cast<struct udev_device*>(udev_dev);
    DeviceInfo info;

    const char* action = ::udev_device_get_action(dev);
    const char* subsystem = ::udev_device_get_subsystem(dev);
    const char* devnode = ::udev_device_get_devnode(dev);
    const char* vendor_id = ::udev_device_get_property_value(dev, "ID_VENDOR_ID");
    const char* product_id = ::udev_device_get_property_value(dev, "ID_MODEL_ID");

    if (action) info.action = action;
    if (subsystem) info.subsystem = subsystem;
    if (devnode) info.devnode = devnode;
    if (vendor_id) info.vendor_id = vendor_id;
    if (product_id) info.product_id = product_id;

    return info;
}

bool MainLoop::ScanAllDevice() {
    struct udev_enumerate* enumerate = udev_enumerate_new(udev_);
    if (!enumerate) {
        log_error("  udev_enumerate_new() 失败");
        return false;
    }

    ::udev_enumerate_add_match_subsystem(enumerate, "usb");
    ::udev_enumerate_add_match_subsystem(enumerate, "tty");
    ::udev_enumerate_add_match_subsystem(enumerate, "net");

    int ret = ::udev_enumerate_scan_devices(enumerate);
    if (ret != 0) {
        log_error("  udev_enumerate_scan_devices() 失败");
        udev_enumerate_unref(enumerate);
        return false;
    }

    struct udev_list_entry* devs = ::udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;
    udev_list_entry_foreach(entry, devs) {
        const char* syspath = ::udev_list_entry_get_name(entry);
        struct udev_device* dev = ::udev_device_new_from_syspath(udev_, syspath);

        DeviceInfo info = ParseDeviceEvent(dev);
        ::udev_device_unref(dev);
        if (info.devnode.empty()) {
            continue;
        }

        AddSensor(info.subsystem, info.vendor_id, info.product_id, info.devnode);
    }

    ::udev_enumerate_unref(enumerate);
    sacndevice_ = true;
    return true;
}

// ==================== 驱动匹配 ====================

const DriverEntry* MainLoop::FindDriver(const std::string& subsystem,
                                        const std::string& vendor_id,
                                        const std::string& product_id) {
    for (const auto& entry : driver_registry_) {
        if (entry.subsystem != subsystem) continue;
        if (!entry.vendor_id.empty() && entry.vendor_id != vendor_id) continue;
        if (!entry.product_id.empty() && entry.product_id != product_id) continue;
        return &entry;
    }
    return nullptr;
}

// ==================== 传感器调度 ====================

void MainLoop::AddSensor(const std::string& subsystem,
                         const std::string& vendor_id,
                         const std::string& product_id,
                         const std::string& devnode) {
    /* 检查是否已存在 */
    if (slotMap_.count(devnode)) {
        log_info("[Manager] 传感器 %s 已存在", devnode.c_str());
        return;
    }

    /* 匹配驱动 */
    const DriverEntry* entry = FindDriver(subsystem, vendor_id, product_id);
    if (!entry) {
        if (sacndevice_) {
            log_info("[Manager] 驱动监听未被注册 (子系统=%s, VID=%s, PID=%s)",
                   subsystem.c_str(), vendor_id.c_str(), product_id.c_str());
        }
        return;
    }

    /* 工厂创建驱动实例 */
    std::unique_ptr<SensorBase> driver = smw::CreateObject<SensorBase>(entry->driver_name);
    if (!driver) {
        log_error("[Manager] 驱动创建失败");
        return;
    }

    /* 注入回调和设备信息 */
    driver->SetName(entry->driver_name);
    driver->SetDevonode(devnode);
    driver->SetDataCallback(entry->onData);
    driver->SetErrorCallback(entry->onError);

    /* 轮询选择一个 SubLoop */
    SubLoop* ioLoop = static_cast<SubLoop*>(ioThreadPool_.getNextLoop());

    /* 构建 SensorSlot，投递给 SubLoop */
    auto slot = std::make_shared<SubLoop::SensorSlot>();
    slot->devnode = devnode;
    slot->driver_name = entry->driver_name;
    slot->driver = std::move(driver);

    /* 记录映射 */
    slotMap_[devnode] = ioLoop;

    /* 投递给 SubLoop：Init → Start → 注册 fd，全在 SubLoop 线程完成 */
    ioLoop->queueInLoop([ioLoop, slot]() {
        ioLoop->addSensorSlot(slot);
    });

    log_info("\n[Manager] 传感器 %s 已投递到 SubLoop (驱动=%s, SubLoop=%p)",
           devnode.c_str(), entry->driver_name.c_str(), ioLoop);
}

void MainLoop::RemoveSensor(const std::string& devnode) {
    auto it = slotMap_.find(devnode);
    if (it == slotMap_.end()) {
        log_info("[Manager] 传感器 %s 不存在", devnode.c_str());
        return;
    }

    SubLoop* ioLoop = it->second;
    slotMap_.erase(it);

    /* 投递给 SubLoop：Stop → Release → 移除，全在 SubLoop 线程完成 */
    ioLoop->queueInLoop([ioLoop, devnode]() {
        ioLoop->removeSensorSlot(devnode);
    });

    log_info("[Manager] 传感器 %s 移除指令已投递到 SubLoop", devnode.c_str());
}

// ==================== 查询 ====================

void MainLoop::ListSensors() const {
    /* 向每个 SubLoop 投递查询，通过 future 等待结果 */
    std::vector<SubLoop::SensorInfo> allInfo;

    for (auto* loop : ioThreadPool_.getAllLoops()) {
        auto* subLoop = static_cast<SubLoop*>(loop);
        auto promise = std::make_shared<std::promise<std::vector<SubLoop::SensorInfo>>>();
        auto future = promise->get_future();

        subLoop->queueInLoop([subLoop, promise]() {
            promise->set_value(subLoop->getSensorInfo());
        });

        auto infos = future.get();  // 阻塞等待 SubLoop 返回
        allInfo.insert(allInfo.end(), infos.begin(), infos.end());
    }

    log_info("\n========= 当前传感器列表 (%zu 个) =========", allInfo.size());
    for (auto& info : allInfo) {
        const char* st = "未知";
        switch (info.status) {
            case SensorStatus::kNull: st = "未创建"; break;
            case SensorStatus::kDefault: st = "未初始化"; break;
            case SensorStatus::kReady: st = "就绪"; break;
            case SensorStatus::kCapturing: st = "采集中"; break;
            case SensorStatus::kError: st = "故障"; break;
        }
        log_info("  %s -> %s [%s] (fd=%d)",
               info.devnode.c_str(), info.driver_name.c_str(), st, info.fd);
    }
    printf("==========================================\n\n");
}

} // namespace eventloop
} // namespace smw
