/**
 * @brief WT61基础驱动
 * @date  2026.07.18
 */

#include "WT61.h"

#include <fcntl.h>
#include <unistd.h> 
#include <termios.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <algorithm>

WT61::WT61(const std::string& name, const std::string& devnode)
    : SensorBase(name,devnode), fd_(-1) { }
WT61::WT61() : fd_(-1) { }

WT61:: ~WT61() {
    if(fd_ >= 0) Release();
 }

int WT61::Init(){
    log_info("[Serial:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());
    fd_ = open(devnode_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd_ < 0) {
        log_error("[Serial:%s] 打开失败: %s", name_.c_str(), strerror(errno));
        status_ = SensorStatus::kError;
        return -1;
    }

    /* 配置串口 115200 8N1 */
    struct termios tty {};
    tcgetattr(fd_, &tty);
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8 | CLOCAL | CREAD;
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // 100ms
    tcsetattr(fd_, TCSANOW, &tty);

    int ret = write(fd_, unlockCmd, 5);
    usleep(150 * 1000);
    ret += write(fd_, speedCmd, 5);
    usleep(150 * 1000);
    ret += write(fd_, saveCmd, 5);
    if(ret != 15) {
        log_error("[Serial:%s] 速率配置失败: %s", name_.c_str(), strerror(errno));
        status_ = SensorStatus::kError;
        return -1;
    } 

    status_ = SensorStatus::kReady;
    log_info("[Serial:%s] 初始化成功 (fd=%d)", name_.c_str(), fd_);
    return 0;
}

int WT61::Start(){
    if(status_ != SensorStatus::kReady) {
        log_error("[Serial:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    status_ = SensorStatus::kCapturing;
    log_info("[Serial:%s] 已启动采集 (fd=%d)", name_.c_str(), fd_);
    return 0;
}

int WT61::Stop(){
    status_  = SensorStatus::kReady;
    log_info("[Serial:%s] 已停止采集", name_.c_str());
    return 0;
}

int WT61::Release(){
    if(fd_ >= 0){
        close(fd_);
        fd_ = -1;
    }
    status_ = SensorStatus::kDefault;
    log_info("[Serial:%s] 已释放", name_.c_str());
    return 0;
}

int WT61::ReadData() {
    if (fd_ < 0) return -1;

    ssize_t n = ::read(fd_, buffer_ + bufIdx, 512);
    if (n > 0) {
        bufIdx += n;
        auto tmp = std::make_shared<ImuData>();
        if(parseFullImuFrame(buffer_, bufIdx, *tmp))
            pushData(tmp);
    }
    else if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        log_error("[Serial:%s] 读取错误: %s", name_.c_str(), strerror(errno));
        if (on_error_) {
            on_error_(name_, errno);
        }
        status_ = SensorStatus::kError;
        return -1;
    }
    return 0;
}

bool WT61::parseFullImuFrame(const uint8_t* buf, int bufLen, ImuData& out) {
    int startIdx = -1;
    for (int i = 0; i < bufLen - 1; i++) {
        if (buf[i] == 0x55 && buf[i+1] == 0x51) {       // 匹配帧头
            if(((bufLen - i) < (8 * WT_PACK_LEN)) &&    // 最后一帧
                (bufLen - i) >= (4 * WT_PACK_LEN)) {    // 完整帧
                startIdx = i;
                bufIdx = bufLen - i - 4 * WT_PACK_LEN;
                break;
            }
            else i += 4 * WT_PACK_LEN;
        }
    }
    if (startIdx == -1)
        return false;

    int pos = startIdx;
    bool findAcc = false, findGyro = false, findAngle = false;

    while (pos < startIdx + 4 * WT_PACK_LEN) {
        if (buf[pos] == 0x55) {
            const uint8_t* pkt = buf + pos;
            if (parseWtSinglePacket(pkt, out)) {
                switch (pkt[1]) {
                    case 0x51: findAcc = true; break;
                    case 0x52: findGyro = true; break;
                    case 0x53: findAngle = true; break;
                }
                pos += WT_PACK_LEN;
                continue;
            }
        }
        pos++;
    }
    std::copy_n(buffer_ + startIdx + 4 * WT_PACK_LEN, bufIdx, buffer_);
    return findAcc && findGyro && findAngle;
}

bool WT61::parseWtSinglePacket(const uint8_t* pkt, ImuData& out) {
    if (pkt[0] != 0x55)
        return false;

    // 累加和校验 0~9字节求和 &0xFF
    // 55 51 EA FF 1C 00 31 08 C9 0E BB
    uint16_t sum = 0;
    for (int i = 0; i < 10; i++) sum += pkt[i];
    uint8_t calcSum = static_cast<uint8_t>(sum & 0xFF);
    if (calcSum != pkt[10]) return false;

    uint8_t type = pkt[1];
    int16_t x = getInt16(pkt[2], pkt[3]);
    int16_t y = getInt16(pkt[4], pkt[5]);
    int16_t z = getInt16(pkt[6], pkt[7]);

    switch (type)
    {
        case 0x51: // 加速度 ±16g
            out.accelerometer_x = static_cast<float>(x) / 32768.0f * 16.0f * 9.8f;
            out.accelerometer_y = static_cast<float>(y) / 32768.0f * 16.0f * 9.8f;
            out.accelerometer_z = static_cast<float>(z) / 32768.0f * 16.0f * 9.8f;
            break;
        case 0x52: // 角速度 ±2000°/s
            out.gyroscope_x = static_cast<float>(x) / 32768.0f * 2000.0f;
            out.gyroscope_y = static_cast<float>(y) / 32768.0f * 2000.0f;
            out.gyroscope_z = static_cast<float>(z) / 32768.0f * 2000.0f;
            break;
        case 0x53: // 欧拉角 ±180°
            out.roll = static_cast<float>(x) / 32768.0f * 180.0f;
            out.pitch = static_cast<float>(y) / 32768.0f * 180.0f;
            out.yaw = static_cast<float>(z) / 32768.0f * 180.0f;
            break;
        default:
            // out.valid = false;
            return false;
    }
    return true;
}
