/**
 * @brief: MainLoop —— 主线程事件循环 + 传感器管理（合并自 SensorManger）
 *
 * 职责：
 *   1. 运行主线程事件循环（poll 监听 udev fd）
 *   2. udev 热插拔监控
 *   3. 驱动注册与匹配
 *   4. 传感器调度：匹配 → 创建 → 注入回调 → 投递给 SubLoop
 *   5. 管理 SubLoop 线程池
 *
 * 所有权模型：
 *   MainLoop 只持有 devnode → SubLoop* 的映射（不拥有 driver）
 *   SubLoop 通过 shared_ptr 持有 driver，管理完整生命周期
 *   MainLoop 不直接访问 driver，无跨线程问题，无需锁
 */

#ifndef SMW_EVENTLOOOP_MAINLOOP_H_
#define SMW_EVENTLOOOP_MAINLOOP_H_

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SubLoop.h"
#include "../Sensor_driver.h"

#include <map>
#include <string>
#include <vector>
#include <libudev.h>

namespace smw       {
namespace eventloop {

class MainLoop : public EventLoop {
public:
    explicit MainLoop(PollerType type = PollerType::kPoll);
    ~MainLoop();

    /* 配置（必须在 Start() 之前调用）*/
    void setThreadNum(int num);

    /* 驱动注册（main 函数调用）*/
    void RegisterDriver(const DriverEntry& entry);

    /* 启动 / 停止 */
    int Start();
    void Stop();

    /* 查询（通过 queueInLoop 同步查询 SubLoop，低频操作）*/
    void ListSensors() const;

protected:
    void handleEvents(Poller* poller) override;

private:
    bool handleUdevEvent();  // 返回 true 表示处理了事件，false 表示没有更多事件
    bool ScanAllDevice();

    struct DeviceInfo {
        std::string action;
        std::string subsystem;
        std::string devnode;
        std::string vendor_id;
        std::string product_id;
    };
    DeviceInfo ParseDeviceEvent(void* udev_dev);

    const DriverEntry* FindDriver(const std::string& subsystem,
                                  const std::string& vendor_id,
                                  const std::string& product_id);

    void AddSensor(const std::string& subsystem,
                   const std::string& vendor_id,
                   const std::string& product_id,
                   const std::string& devnode);
    void RemoveSensor(const std::string& devnode);

    /* 成员 */
    std::vector<DriverEntry> driver_registry_;
    std::map<std::string, SubLoop*> slotMap_;
    EventLoopThreadPool ioThreadPool_;

    ::udev* udev_;
    ::udev_monitor* udev_mon_;
    bool sacndevice_;
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_MAINLOOP_H_
