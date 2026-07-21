/**
 * @brief LIJU-hyxw-x6 六维力/力矩传感器驱动
 * @date  2026.07.20
 *
 * 从 tmp/Power/LIJU-hyxw-x6 标准化移植，适配 SensorBase + timerfd 定时写调度
 * 请求-响应型：WriteData() 发命令，ReadData() 收 27 字节响应
 */

#include "LIJU.h"

#include <cstdio>
#include <cstring>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

/* 获取解耦数据命令 */
const uint8_t LIJU::kCmdGetData[8] = {0x01, 0x03, 0x00, 0xD4, 0x00, 0x0C, 0x05, 0xF7};

// ===================== 构造 / 析构 =====================

LIJU::LIJU(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), respIdx_(0)
{
    memset(respBuf_, 0, sizeof(respBuf_));
}

LIJU::LIJU()
    : respIdx_(0)
{
    memset(respBuf_, 0, sizeof(respBuf_));
}

LIJU::~LIJU() {
    if (serial_.IsOpened()) Release();
}

// ===================== 生命周期 =====================

int LIJU::Init() {
    log_info("[Power:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    int fd = serial_.Open(devnode_, 19200);
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

int LIJU::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[Power:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    respIdx_ = 0;
    status_ = SensorStatus::kCapturing;
    requestWrite();  // 请求第一次写
    log_info("[Power:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int LIJU::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[Power:%s] 已停止采集", name_.c_str());
    return 0;
}

int LIJU::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[Power:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 请求-响应 =====================

/* 由 MainLoop timerfd 通过 queueInLoop 调度 */
int LIJU::WriteData() {
    int ret = serial_.Write(kCmdGetData, 8);
    if (ret < 0) {
        log_error("[Power:%s] 发送命令失败", name_.c_str());
    }
    markWriteDone();
    return 0;
}

/* 由 SubLoop POLLIN 触发，增量读取 27 字节响应 */
int LIJU::ReadData() {
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
    // 通道顺序：Mx(1) My(2) Mz(3) fx(4) fy(5) fz(6)
    uint32_t raw[6];
    for (int i = 0; i < 6; i++) {
        raw[i] = composeU32(respBuf_, 3 + i * 4);
    }

    auto pd = std::make_shared<PowerData>();
    pd->devName = name_;
    pd->frequency = 1;

    // 原始代码映射：calc_buffer[0]=fx, [1]=fy, [2]=fz, [3]=Mx, [4]=My, [5]=Mz
    // Compon_data 顺序：1→Mx, 2→My, 3→Mz, 4→fx, 5→fy, 6→fz
    pd->Mx = twosComplementToFloat(raw[0]);
    pd->My = twosComplementToFloat(raw[1]);
    pd->Mz = twosComplementToFloat(raw[2]);
    pd->fx = twosComplementToFloat(raw[3]);
    pd->fy = twosComplementToFloat(raw[4]);
    pd->fz = twosComplementToFloat(raw[5]);

    pushData(pd);

    respIdx_ = 0;
    requestWrite();  // 请求下一次写
    return 0;
}

// ===================== 数据解析 =====================

uint32_t LIJU::composeU32(const uint8_t* buf, int offset) {
    return (uint32_t)buf[offset] << 24
         | (uint32_t)buf[offset + 1] << 16
         | (uint32_t)buf[offset + 2] << 8
         | (uint32_t)buf[offset + 3];
}

float LIJU::twosComplementToFloat(uint32_t value) {
    if (value & 0x80000000) {
        // 负数：补码 → 原码
        uint32_t fanma = value - 1;
        int32_t yuanma = 0;
        for (int i = 0; i < 31; i++) {
            uint32_t bit = (~((fanma >> i) & 1)) & 1;
            yuanma += bit << i;
        }
        if (fanma & 0x80) yuanma = -yuanma;
        return (float)yuanma / 100.0f;
    }
    // 正数：直接除
    return (float)value / 100.0f;
}
