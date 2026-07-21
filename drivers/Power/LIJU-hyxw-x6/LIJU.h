/**
 * @brief LIJU-hyxw-x6 六维力/力矩传感器驱动
 * @date  2026.07.20
 *
 * 协议：Modbus 风格，发 8 字节命令 → 收 27 字节响应
 *       6 个通道（Mx/My/Mz/fx/fy/fz），每通道 4 字节大端补码，÷100
 * 波特率：19200
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"
#include "../../../smw/common/ttySerial.h"

#include <cstdint>
#include <string>

using namespace smw;

class LIJU : public SensorBase {
public:
    LIJU(const std::string& name, const std::string& devnode);
    LIJU();
    ~LIJU() override;

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    int fd() const override { return serial_.fd(); }
    int ReadData() override;
    int WriteData() override;

private:
    /* 4 字节大端组合为 unsigned long */
    static uint32_t composeU32(const uint8_t* buf, int offset);

    /* 补码 → 有符号浮点（÷100） */
    static float twosComplementToFloat(uint32_t value);

    ttySerial serial_;

    /* 获取解耦数据命令（Modbus Read Holding Registers） */
    static const uint8_t kCmdGetData[8];

    static const int kResponseLen = 27;
    uint8_t respBuf_[kResponseLen];
    int respIdx_;
};

CLASS_LOADER_REGISTER_CLASS(LIJU, SensorBase)
