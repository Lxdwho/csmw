/**
 * @brief CASIA 触觉传感器 V2 驱动
 * @date  2026.07.20
 *
 * 从 tmp/Touch/Touch_test_2 标准化移植，适配 SensorBase + SubLoop 事件驱动架构
 * 协议：':' 起始 + 36 个 6 字符浮点值（6×6 网格）
 */

#include "CasTouchV2.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

// ===================== 构造 / 析构 =====================

CasTouchV2::CasTouchV2(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), baudRate_(576000), rawIdx_(0)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
}

CasTouchV2::CasTouchV2()
    : baudRate_(576000), rawIdx_(0)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
}

CasTouchV2::~CasTouchV2() {
    if (serial_.IsOpened()) Release();
}

// ===================== 生命周期 =====================

int CasTouchV2::Init() {
    log_info("[TouchV2:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    IniFile* ini = IniFile::Get_instance();
    if (ini && ini->has(name_, "baud_rate")) {
        baudRate_ = (int)ini->get(name_, "baud_rate");
    }

    int fd = serial_.Open(devnode_, baudRate_);
    if (fd < 0) {
        log_error("[TouchV2:%s] 打开失败", name_.c_str());
        status_ = SensorStatus::kError;
        return -1;
    }

    status_ = SensorStatus::kReady;
    log_info("[TouchV2:%s] 初始化成功 (fd=%d, baud=%d)", name_.c_str(), fd, baudRate_);
    return 0;
}

int CasTouchV2::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[TouchV2:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    rawIdx_ = 0;
    status_ = SensorStatus::kCapturing;
    log_info("[TouchV2:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int CasTouchV2::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[TouchV2:%s] 已停止采集", name_.c_str());
    return 0;
}

int CasTouchV2::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[TouchV2:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 数据读取 =====================

int CasTouchV2::ReadData() {
    if (!serial_.IsOpened()) return -1;

    int n = serial_.Read(rawBuf_ + rawIdx_, kBufSize - rawIdx_ - 1);
    if (n < 0) {
        log_error("[TouchV2:%s] 读取错误", name_.c_str());
        if (on_error_) on_error_(name_, errno);
        status_ = SensorStatus::kError;
        return -1;
    }
    if (n == 0) return 0;

    rawIdx_ += n;
    rawBuf_[rawIdx_] = '\0';

    // 查找 ':' 起始符
    int colonPos = -1;
    for (int i = 0; i < rawIdx_; i++) {
        if (rawBuf_[i] == ':') {
            colonPos = i;
            break;
        }
    }

    if (colonPos < 0) {
        rawIdx_ = 0;
        return 0;
    }

    // 等待至少 kDataSize 字节有效数据（36×6=216）
    int dataAvail = rawIdx_ - colonPos - 1;
    if (dataAvail < kDataSize) {
        if (colonPos > 0) {
            int remain = rawIdx_ - colonPos;
            memmove(rawBuf_, rawBuf_ + colonPos, remain);
            rawIdx_ = remain;
        }
        return 0;
    }

    // 解析 36 个点
    const char* data = (const char*)rawBuf_ + colonPos + 1;
    char numbuf[7] = {0};
    auto td = std::make_shared<tactile2Data>();
    td->devName = name_;
    td->frequency = 1;
    td->points.resize(kNumPoints);

    for (int i = 0; i < kDataSize; i += 6) {
        int idx = i / 6;
        memcpy(numbuf, data + i, 6);
        numbuf[6] = '\0';
        td->points[idx].x = idx % kGridW;
        td->points[idx].y = idx / kGridW;
        td->points[idx].tactile = atof(numbuf);
    }

    pushData(td);

    // 前移：跳过 ':' + kFrameSize 字节（传感器每帧发 251 字节）
    int consumed = colonPos + 1 + kFrameSize;
    if (consumed < rawIdx_) {
        int remain = rawIdx_ - consumed;
        memmove(rawBuf_, rawBuf_ + consumed, remain);
        rawIdx_ = remain;
    } else {
        rawIdx_ = 0;
    }

    return 0;
}
