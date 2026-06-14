/**
 * @brief: SubLoop —— 工作线程事件循环 + 传感器 I/O
 *
 * 职责：
 *   1. 运行工作线程事件循环（poll 监听分配到的传感器 fd）
 *   2. 持有 driver 的唯一所有权，管理完整生命周期：
 *      Init → Start → ReadData → Stop → Release
 *   3. 注册/注销传感器 fd
 *   4. 读取传感器数据并触发回调
 *
 * 线程模型：
 *   ownedSlots_ 的所有操作都在 SubLoop 自己的线程中执行（通过 queueInLoop 投递）
 *   MainLoop 不直接访问 driver，无需任何锁
 */

#ifndef SMW_EVENTLOOOP_SUBLOOP_H_
#define SMW_EVENTLOOOP_SUBLOOP_H_

#include "EventLoop.h"
#include "../Sensor_driver.h"

#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace smw       {
namespace eventloop {

class SubLoop : public EventLoop {
public:
    /**
     * @brief: 传感器 Slot —— 包含驱动实例及其运行时状态
     * SubLoop 独占持有，MainLoop 不直接访问 driver
     */
    struct SensorSlot {
        std::string devnode;
        std::string driver_name;
        std::unique_ptr<SensorBase> driver;
    };

    explicit SubLoop(PollerType type = PollerType::kPoll);
    ~SubLoop();

    /**
     * @brief: 接管传感器（在 SubLoop 线程中执行）
     * 完成 Init → Start → 注册 fd
     * 成功后传感器开始被 poll 监听
     */
    void addSensorSlot(std::shared_ptr<SensorSlot> slot);

    /**
     * @brief: 移除传感器（在 SubLoop 线程中执行）
     * 完成 Stop → Release → 从 ownedSlots_ 移除
     */
    void removeSensorSlot(const std::string& devnode);

    /* 传感器信息查询（在 SubLoop 线程中执行，供 MainLoop 通过 queueInLoop 调用）*/
    struct SensorInfo {
        std::string devnode;
        std::string driver_name;
        SensorStatus status;
        int fd;
    };
    std::vector<SensorInfo> getSensorInfo() const;

protected:
    void registerFds(Poller* poller) override;
    void handleEvents(Poller* poller) override;

private:
    std::vector<std::shared_ptr<SensorSlot>> ownedSlots_;
    std::map<int, size_t> fdToSlotMap_;  // fd → ownedSlots_ 索引，用于快速查找
};

} // namespace eventloop
} // namespace smw

#endif // SMW_EVENTLOOOP_SUBLOOP_H_
