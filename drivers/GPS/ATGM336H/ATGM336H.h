/**
 * @brief ATGM336H GPS 模块驱动
 * @date  2026.07.20
 *
 * NMEA 语句解析：$GNRMC（经纬度）+ $GNGGA（海拔/卫星/精度）+ $GNVTG（航向/速率）
 * 三句齐全后 pushData 一帧 GpsData
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"
#include "../../../smw/common/ttySerial.h"

#include <cstdint>
#include <string>

using namespace smw;

class ATGM336H : public SensorBase {
public:
    ATGM336H(const std::string& name, const std::string& devnode);
    ATGM336H();
    ~ATGM336H() override;

    // 生命周期
    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    // SubLoop 接口
    int fd() const override { return serial_.fd(); }
    int ReadData() override;

private:
    /* 语句解析 */
    bool parseGNRMC(const char* sentence, GpsData& out);
    bool parseGNGGA(const char* sentence, GpsData& out);
    bool parseGNVTG(const char* sentence, GpsData& out);

    /* 辅助：以 ',' 分隔提取第 n 个字段（0-based），写入 buf，返回是否成功 */
    static bool extractField(const char* sentence, int index, char* buf, int bufSize);

    /* NMEA 度分 → 十进制度 */
    static double nmeaToDegrees(const char* nmea);

    ttySerial serial_;
    int baudRate_;

    static const int kRawBufSize = 1024;
    uint8_t rawBuf_[kRawBufSize];   // 原始接收缓冲
    int rawIdx_;                     // 缓冲区有效字节数

    // 最近一次解析到的完整 NMEA 语句
    char gnrmc_[128];
    char gngga_[128];
    char gnvtg_[128];
    bool hasRmc_;
    bool hasGga_;
    bool hasVtg_;
};

CLASS_LOADER_REGISTER_CLASS(ATGM336H, SensorBase)
