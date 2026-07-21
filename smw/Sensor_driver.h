/**
 * @brief: 传感器驱动基类,注册工程父类
 * @date 2026.06.13
 * 支持推拉结合模式：
 *   推：注册 onData 回调，数据到达时自动触发
 *   拉：调用 readLatest() 主动获取最新数据
 *   两者可同时使用，互不干扰
 */

#ifndef _SMW_SENSORDRIVER_H_
#define _SMW_SENSORDRIVER_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include "./classFactory/ClassFactory.h"
#include "DataBase.h"

namespace smw {

/* 传感器状态 */
enum class SensorStatus {
    kNull = 0,  //未创建
    kDefault,   //已创建未初始化
    kReady,     //已初始化
    kCapturing, //采集中
    kError,     //故障
};

/* 回调类型 */
using OnSensorData = std::function<void(const DataBase*)>;
using OnSensorError = std::function<void(const std::string& name, int error_code)>;

/**
 * @brief: 传感器驱动基类
 *
 * 子类在 ReadData() 中解析数据后，调用 pushData() 而非直接调 on_data_
 * pushData() 同时完成：触发回调（推）+ 更新缓冲区（拉）
 */
class SensorBase {
public:
    SensorBase(const std::string& name, const std::string& devnode)
        : name_(name), devnode_(devnode), status_(SensorStatus::kDefault) {}
    SensorBase() : status_(SensorStatus::kDefault) {}
    virtual ~SensorBase() = default;

    /* 传感器接口定义 */
    virtual int Init() = 0;
    virtual int Start() = 0;
    virtual int Stop() = 0;
    virtual int Release() = 0;

    /* SubLoop 接口：用于 poll 监听 */
    virtual int fd() const { return -1; }
    virtual int ReadData() { return 0; }

    /* 请求-响应传感器接口 */
    virtual bool wantsWrite() const {
        if (!writeRequested_) return false;
        if (writeIntervalMs_ <= 0) return true;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastWriteTime_).count();
        return elapsed >= writeIntervalMs_;
    }
    virtual int WriteData() { return 0; }

    /* 请求-响应调度 API */
    void requestWrite() { writeRequested_ = true; }
    void setWriteInterval(int ms) { writeIntervalMs_ = ms; }
    int getWriteInterval() const { return writeIntervalMs_; }

    /* 状态查询 */
    SensorStatus GetStatus() const { return status_; }
    const std::string& GetName() const { return name_; }
    const std::string& GetDevnode() const { return devnode_; }
    void SetName(const std::string& name) { name_ = name; }
    void SetDevonode(const std::string& devnode) { devnode_ = devnode; }

    /* 推模式：注入回调 */
    void SetDataCallback(OnSensorData cb) { on_data_ = std::move(cb); }
    void SetErrorCallback(OnSensorError cb) { on_error_ = std::move(cb); }

    /* 拉模式：获取最新数据
     * 返回 true 表示有新数据，false 表示没有更新
     * out 是 shared_ptr<DataBase>，用 dynamic_cast 转为具体类型
     */
    bool readLatest(std::shared_ptr<DataBase>& out) {
        std::lock_guard<std::mutex> lock(dataMtx_);
        if (!latestData_) return false;
        out = latestData_;
        latestData_.reset();
        return true;
    }

    /* 拉模式：获取最新数据（非破坏性，不清除新数据标志）
     * 多次调用返回同一份数据，直到下一次 pushData 更新
     */
    bool peekLatest(std::shared_ptr<DataBase>& out) const {
        std::lock_guard<std::mutex> lock(dataMtx_);
        if (!latestData_) return false;
        out = latestData_;
        return true;
    }

protected:
    std::string name_;
    std::string devnode_;
    std::atomic<SensorStatus> status_;
    OnSensorData on_data_;
    OnSensorError on_error_;

    /**
     * @brief: 推拉结合 —— 子类在 ReadData 中调用此函数
     *
     * 1. 触发 on_data_ 回调（推）
     * 2. 更新 latestData_ 缓冲区（拉）
     *
     * 用法：pushData(std::make_shared<ImuData>(data));
     */
    void pushData(std::shared_ptr<DataBase> data) {
        // 推：触发回调
        if (on_data_) {
            on_data_(data.get());
        }
        // 拉：更新最新数据缓冲区
        std::lock_guard<std::mutex> lock(dataMtx_);
        latestData_ = std::move(data);
    }

    /* 子类在 WriteData() 中调用，标记写完成 */
    void markWriteDone() {
        writeRequested_ = false;
        lastWriteTime_ = std::chrono::steady_clock::now();
    }

private:
    mutable std::mutex dataMtx_;
    std::shared_ptr<DataBase> latestData_;  // 最新一帧数据（拉模式用）

    /* 请求-响应节流 */
    mutable bool writeRequested_ = false;
    int writeIntervalMs_ = 0;
    std::chrono::steady_clock::time_point lastWriteTime_;
};

/* 驱动注册表 */
struct DriverEntry {
    std::string subsystem;
    std::string vendor_id;
    std::string product_id;
    std::string driver_name;
    std::string interface;     // 网络传感器：匹配网口名（如 "eth1"），空表示串口
    int writeIntervalMs = 0;   // 请求-响应型传感器写间隔（ms），0 = 纯流式
    OnSensorData onData;
    OnSensorError onError;
};

} // namespace smw

#endif // _SMW_SENSORDRIVER_H_
