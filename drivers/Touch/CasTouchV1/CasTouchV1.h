/**
 * @brief CASIA 触觉传感器 V1 驱动
 * @date  2026.07.20
 *
 * 协议：':' 起始 + 118 字节（24 个 6 字符浮点值）
 *       前 5 个为红外，后 19 个为触觉
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

class CasTouchV1 : public SensorBase {
public:
    CasTouchV1(const std::string& name, const std::string& devnode);
    CasTouchV1();
    ~CasTouchV1() override;

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    int fd() const override { return serial_.fd(); }
    int ReadData() override;

private:
    static const int kFrameSize = 118;  // 24 个值 × 6 字符 - 2（去掉前两个字符）
    static const int kBufSize = 512;

    ttySerial serial_;
    int baudRate_;

    uint8_t rawBuf_[kBufSize];
    int rawIdx_;
};

CLASS_LOADER_REGISTER_CLASS(CasTouchV1, SensorBase)
