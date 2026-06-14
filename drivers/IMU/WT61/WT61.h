/**
 * @brief:CH340串口重写sensor_driver基类?
 * 用户态驱动框架开发
 */
#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"

using namespace smw;

/* 继承 + 虚函数重写 */
/* 无论是哪种类型的设备（USB\NET)都继承这个SensorBase基类，因为只需要实现这个基类的四个接口功能就行. */
class WT61 :  public SensorBase {
public:
    WT61(const std::string& name, const std::string& devnode);
    WT61();
    ~WT61() override;

    // 生命周期
    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    /* SubLoop 接口 */
    int fd() const override { return fd_; }
    int ReadData() override;  // 非阻塞读取，触发 on_data_ 回调

private:
    int fd_;
};

/*
 * 向 SensorBase 工厂注册 WT61 子类，
    (把 WT61 的创建方法注册给工厂)
 * 后续可通过字符串动态创建 WT61 对象(可多个)
 */
CLASS_LOADER_REGISTER_CLASS(WT61, SensorBase)