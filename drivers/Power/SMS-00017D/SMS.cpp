/**
 * @brief SMS-00017D.R304 六维力/力矩传感器驱动
 * @date  2026.07.20
 *
 * 从 tmp/Power/SMS-00017D.R304 标准化移植，适配 SensorBase + timerfd 定时写调度
 * 请求-响应型：WriteData() 发命令，ReadData() 收 29 字节响应
 */

#include "SMS.h"

#include <cstdio>
#include <cstring>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

/* 获取数据命令 */
const uint8_t SMS::kCmdGetData[8] = {0x01, 0x03, 0x04, 0x00, 0x00, 0x0C, 0x44, 0xFF};

// ===================== 构造 / 析构 =====================

SMS::SMS(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), respIdx_(0)
{
    memset(respBuf_, 0, sizeof(respBuf_));
}

SMS::SMS()
    : respIdx_(0)
{
    memset(respBuf_, 0, sizeof(respBuf_));
}

SMS::~SMS() {
    if (serial_.IsOpened()) Release();
}

// ===================== 生命周期 =====================

int SMS::Init() {
    log_info("[Power:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    int fd = serial_.Open(devnode_, 115200);
    if (fd < 0) {
        log_error("[Power:%s] 打开失败", name_.c_str());
        status_ = SensorStatus::kError;
        return -1;
    }

    /* 从 INI 读取写间隔（默认 100ms） */
    int writeMs = 100;
    IniFile* ini = IniFile::Get_instance();
    if (ini && ini->has(name_, "write_interval")) {
        writeMs = (int)ini->get(name_, "write_interval");
    }
    setWriteInterval(writeMs);

    status_ = SensorStatus::kReady;
    log_info("[Power:%s] 初始化成功 (fd=%d, write_interval=%d ms)", name_.c_str(), fd, writeMs);
    return 0;
}

int SMS::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[Power:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    respIdx_ = 0;
    status_ = SensorStatus::kCapturing;
    requestWrite();
    log_info("[Power:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int SMS::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[Power:%s] 已停止采集", name_.c_str());
    return 0;
}

int SMS::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[Power:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 请求-响应 =====================

int SMS::WriteData() {
    int ret = serial_.Write(kCmdGetData, 8);
    if (ret < 0) {
        log_error("[Power:%s] 发送命令失败", name_.c_str());
    }
    markWriteDone();
    return 0;
}

int SMS::ReadData() {
    if (!serial_.IsOpened()) return -1;

    int n = serial_.Read(respBuf_ + respIdx_, kResponseLen - respIdx_);
    if (n < 0) {
        log_error("[Power:%s] 读取错误", name_.c_str());
        if (on_error_) on_error_(name_, errno);
        status_ = SensorStatus::kError;
        return -1;
    }
    if (n == 0) return 0;

    respIdx_ += n;
    if (respIdx_ < kResponseLen) {
        return 0;  // 数据不够，等下次 POLLIN
    }

    // 解析 6 个通道（响应从字节 3 开始，每通道 4 字节大端）
    // 通道顺序：fx(1) fy(2) fz(3) Mx(4) My(5) Mz(6)
    uint32_t raw[6];
    for (int i = 0; i < 6; i++) {
        raw[i] = composeU32(respBuf_, 3 + i * 4);
    }

    auto pd = std::make_shared<PowerData>();
    pd->devName = name_;
    pd->frequency = 1;

    // SMS 的通道顺序：fx fy fz Mx My Mz（与 LIJU 不同）
    pd->fx = hexToFloat(raw[0]);
    pd->fy = hexToFloat(raw[1]);
    pd->fz = hexToFloat(raw[2]);
    pd->Mx = hexToFloat(raw[3]);
    pd->My = hexToFloat(raw[4]);
    pd->Mz = hexToFloat(raw[5]);

    pushData(pd);

    respIdx_ = 0;
    requestWrite();
    return 0;
}

// ===================== 数据解析 =====================

uint32_t SMS::composeU32(const uint8_t* buf, int offset) {
    return (uint32_t)buf[offset] << 24
         | (uint32_t)buf[offset + 1] << 16
         | (uint32_t)buf[offset + 2] << 8
         | (uint32_t)buf[offset + 3];
}

float SMS::hexToFloat(uint32_t hvalue) {
    // 原始代码：fvalue = *(float*)&temp; 即把 uint32 的二进制直接当 float 解释
    float fvalue;
    memcpy(&fvalue, &hvalue, sizeof(float));
    return fvalue;
}
