/**
 * @brief NBIT 六维力/力矩传感器驱动（UDP 网口）
 * @date  2026.07.20
 *
 * 从 tmp/Power/NBIT 标准化移植，适配 SensorBase + timerfd 定时写调度
 * 请求-响应型：WriteData() 发 UDP 命令，ReadData() 收 UDP 响应
 */

#include "NBIT.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

// ===================== 构造 / 析构 =====================

NBIT::NBIT(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), sockfd_(-1), port_(1024)
{
    memset(respBuf_, 0, sizeof(respBuf_));
    memset(sum_, 0, sizeof(sum_));
    initCommands(0x01);
}

NBIT::NBIT()
    : sockfd_(-1), port_(1024)
{
    memset(respBuf_, 0, sizeof(respBuf_));
    memset(sum_, 0, sizeof(sum_));
    initCommands(0x01);
}

NBIT::~NBIT() {
    if (sockfd_ >= 0) Release();
}

void NBIT::initCommands(uint8_t addr) {
    /* 取得数据指令 */
    cmdData_[0] = addr;
    cmdData_[1] = 0x03;
    cmdData_[2] = 0x02;
    cmdData_[3] = 0x00;
    cmdData_[4] = 0x12;
}

// ===================== 生命周期 =====================

int NBIT::Init() {
    log_info("[NBIT:%s] 初始化 (interface=%s)", name_.c_str(), devnode_.c_str());

    /* 从 INI 读取 IP 和端口 */
    IniFile* ini = IniFile::Get_instance();
    if (ini && ini->has(name_, "ip")) {
        ip_ = (std::string)ini->get(name_, "ip");
    } else {
        ip_ = "192.168.1.1";
    }
    if (ini && ini->has(name_, "port")) {
        port_ = (int)ini->get(name_, "port");
    }

    /* 从 INI 读取写间隔（默认 100ms） */
    int writeMs = 100;
    if (ini && ini->has(name_, "write_interval")) {
        writeMs = (int)ini->get(name_, "write_interval");
    }
    setWriteInterval(writeMs);

    /* 创建 UDP socket */
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        log_error("[NBIT:%s] socket 创建失败: %s", name_.c_str(), strerror(errno));
        status_ = SensorStatus::kError;
        return -1;
    }

    /* 设置非阻塞 */
    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

    /* 设置目标地址 */
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ip_.c_str());
    dest.sin_port = htons(port_);

    /* connect UDP：后续 send/recv 不需要每次指定地址 */
    if (connect(sockfd_, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        log_error("[NBIT:%s] connect 失败: %s", name_.c_str(), strerror(errno));
        close(sockfd_);
        sockfd_ = -1;
        status_ = SensorStatus::kError;
        return -1;
    }

    status_ = SensorStatus::kReady;
    log_info("[NBIT:%s] 初始化成功 (fd=%d, %s:%d, write_interval=%d ms)",
             name_.c_str(), sockfd_, ip_.c_str(), port_, writeMs);
    return 0;
}

int NBIT::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[NBIT:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    status_ = SensorStatus::kCapturing;
    requestWrite();
    log_info("[NBIT:%s] 已启动采集 (fd=%d)", name_.c_str(), sockfd_);
    return 0;
}

int NBIT::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[NBIT:%s] 已停止采集", name_.c_str());
    return 0;
}

int NBIT::Release() {
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    status_ = SensorStatus::kDefault;
    log_info("[NBIT:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 请求-响应 =====================

int NBIT::WriteData() {
    int ret = send(sockfd_, cmdData_, 5, 0);
    if (ret < 0) {
        log_error("[NBIT:%s] 发送失败: %s", name_.c_str(), strerror(errno));
    }
    markWriteDone();
    return 0;
}

int NBIT::ReadData() {
    if (sockfd_ < 0) return -1;

    memset(respBuf_, 0, kResponseLen);
    int n = recv(sockfd_, respBuf_, kResponseLen, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  // 没数据
        }
        log_error("[NBIT:%s] 接收失败: %s", name_.c_str(), strerror(errno));
        if (on_error_) on_error_(name_, errno);
        status_ = SensorStatus::kError;
        return -1;
    }
    if (n < kResponseLen) return 0;  // 数据不完整

    /* 解析 6 个通道，每通道 3 字节 */
    int data[6] = {0};
    for (int i = 0; i < 6; i++) {
        int offset = i * 3 + 3;
        bool negative = (respBuf_[offset] & 0x80) != 0;
        if (negative) respBuf_[offset] &= 0x7F;

        for (int j = 0; j < 3; j++) {
            data[i] = (data[i] << 8) + respBuf_[offset + j];
        }

        sum_[i] = data[i] / 1000.0;
        if (negative) sum_[i] = -sum_[i];
    }

    auto pd = std::make_shared<PowerData>();
    pd->devName = name_;
    pd->frequency = 1;
    pd->fx = sum_[0];
    pd->fy = sum_[1];
    pd->fz = sum_[2];
    pd->Mx = sum_[3];
    pd->My = sum_[4];
    pd->Mz = sum_[5];

    pushData(pd);

    requestWrite();
    return 0;
}
