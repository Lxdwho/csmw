/**
 * @brief SMS-00017D.R304 六维力/力矩传感器驱动
 * @date  2026.07.20
 *
 * 协议：Modbus 风格，发 8 字节命令 → 收 29 字节响应
 *       6 个通道（fx/fy/fz/Mx/My/Mz），每通道 4 字节大端 IEEE 754 浮点
 * 波特率：115200
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"
#include "../../../smw/common/ttySerial.h"

#include <cstdint>
#include <string>

using namespace smw;

class SMS : public SensorBase {
public:
    SMS(const std::string& name, const std::string& devnode);
    SMS();
    ~SMS() override;

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    int fd() const override { return serial_.fd(); }
    int ReadData() override;
    int WriteData() override;

private:
    /* 4 字节大端组合为 unsigned int */
    static uint32_t composeU32(const uint8_t* buf, int offset);

    /* IEEE 754 二进制 → float */
    static float hexToFloat(uint32_t hvalue);

    ttySerial serial_;

    /* 获取数据命令 */
    static const uint8_t kCmdGetData[8];

    static const int kResponseLen = 29;
    uint8_t respBuf_[kResponseLen];
    int respIdx_;
};

CLASS_LOADER_REGISTER_CLASS(SMS, SensorBase)
