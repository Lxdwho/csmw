/**
 * @brief WT61基础驱动
 * @date  2026.07.18
 */
#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"

#define WT_PACK_LEN 11

using namespace smw;

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
    int ReadData() override;

    bool parseFullImuFrame(const uint8_t* buf, int bufLen, ImuData& out);
    bool parseWtSinglePacket(const uint8_t* pkt, ImuData& out);

    int16_t getInt16(uint8_t low, uint8_t high) {
        uint16_t raw = (static_cast<uint16_t>(high) << 8) | low;
        return static_cast<int16_t>(raw);
    }

private:
    int fd_;
    uint8_t buffer_[1024];
    uint8_t bufIdx = 0;
    const uint8_t unlockCmd[5] = { 0xFFU, 0xAAU, 0x69U, 0x88U, 0xB5U };
    const uint8_t speedCmd[5]  = { 0xFFU, 0xAAU, 0x03U, 0x08U, 0x00U };
    const uint8_t saveCmd[5]   = { 0xFFU, 0xAAU, 0x00U, 0x00U, 0x00U };
};
CLASS_LOADER_REGISTER_CLASS(WT61, SensorBase)
