/**
 * @brief CASIA 触觉传感器 V2 驱动
 * @date  2026.07.20
 *
 * 协议：':' 起始 + 251 字节（36 个 6 字符浮点值 + 尾部填充）
 *       6×6 网格，每个点有 x/y/tactile
 * 波特率：576000
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"
#include "../../../smw/common/ttySerial.h"

#include <cstdint>
#include <string>

using namespace smw;

class CasTouchV2 : public SensorBase {
public:
    CasTouchV2(const std::string& name, const std::string& devnode);
    CasTouchV2();
    ~CasTouchV2() override;

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    int fd() const override { return serial_.fd(); }
    int ReadData() override;

private:
    static const int kGridW = 6;
    static const int kGridH = 6;
    static const int kNumPoints = kGridW * kGridH;  // 36
    static const int kFrameSize = 251;               // 传感器每帧发送字节数
    static const int kDataSize = kNumPoints * 6;     // 有效数据：36×6 = 216
    static const int kBufSize = 512;

    ttySerial serial_;
    int baudRate_;

    uint8_t rawBuf_[kBufSize];
    int rawIdx_;
};

CLASS_LOADER_REGISTER_CLASS(CasTouchV2, SensorBase)
