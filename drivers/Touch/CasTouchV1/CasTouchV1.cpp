/**
 * @brief CASIA 触觉传感器 V1 驱动
 * @date  2026.07.20
 *
 * 从 tmp/Touch/Touch_test 标准化移植，适配 SensorBase + SubLoop 事件驱动架构
 * 协议：':' 起始 + 24 个 6 字符浮点值（前 5 红外，后 19 触觉）
 */

#include "CasTouchV1.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

// ===================== 构造 / 析构 =====================

CasTouchV1::CasTouchV1(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), baudRate_(115200), rawIdx_(0)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
}

CasTouchV1::CasTouchV1()
    : baudRate_(115200), rawIdx_(0)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
}

CasTouchV1::~CasTouchV1() {
    if (serial_.IsOpened()) Release();
}

// ===================== 生命周期 =====================

int CasTouchV1::Init() {
    log_info("[TouchV1:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    IniFile* ini = IniFile::Get_instance();
    if (ini && ini->has(name_, "baud_rate")) {
        std::string br = (std::string)ini->get(name_, "baud_rate");
        if (!br.empty()) {
            baudRate_ = std::stoi(br);
        }
    }

    int fd = serial_.Open(devnode_, baudRate_);
    if (fd < 0) {
        log_error("[TouchV1:%s] 打开失败", name_.c_str());
        status_ = SensorStatus::kError;
        return -1;
    }

    status_ = SensorStatus::kReady;
    log_info("[TouchV1:%s] 初始化成功 (fd=%d, baud=%d)", name_.c_str(), fd, baudRate_);
    return 0;
}

int CasTouchV1::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[TouchV1:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    rawIdx_ = 0;
    status_ = SensorStatus::kCapturing;
    log_info("[TouchV1:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int CasTouchV1::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[TouchV1:%s] 已停止采集", name_.c_str());
    return 0;
}

int CasTouchV1::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[TouchV1:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 数据读取 =====================

int CasTouchV1::ReadData() {
    if (!serial_.IsOpened()) return -1;

    int n = serial_.Read(rawBuf_ + rawIdx_, kBufSize - rawIdx_ - 1);
    if (n < 0) {
        log_error("[TouchV1:%s] 读取错误", name_.c_str());
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
        // 没找到起始符，丢弃所有数据
        rawIdx_ = 0;
        return 0;
    }

    // 计算 ':' 之后的可用字节数
    int dataAvail = rawIdx_ - colonPos - 1;
    if (dataAvail < kFrameSize) {
        // 数据不够一帧，保留等下次
        if (colonPos > 0) {
            int remain = rawIdx_ - colonPos;
            memmove(rawBuf_, rawBuf_ + colonPos, remain);
            rawIdx_ = remain;
        }
        return 0;
    }

    // 数据足够，解析一帧
    const char* data = (const char*)rawBuf_ + colonPos + 1;
    char numbuf[7] = {0};
    auto td = std::make_shared<tactileData>();
    td->devName = name_;
    td->frequency = 1;

    int nums = 0;
    for (int i = 0; i < kFrameSize && nums < 24; i += 6) {
        memcpy(numbuf, data + i, 6);
        numbuf[6] = '\0';
        if (nums < 5) {
            td->infrared[nums] = atof(numbuf);
        } else {
            td->tactile[nums - 5] = atof(numbuf);
        }
        nums++;
    }

    pushData(td);

    // 前移未处理数据
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
