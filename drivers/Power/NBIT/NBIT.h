/**
 * @brief NBIT 六维力/力矩传感器驱动（UDP 网口）
 * @date  2026.07.20
 *
 * 协议：UDP 发 5 字节命令 → 收 23 字节响应
 *       6 个通道（fx/fy/fz/Mx/My/Mz），每通道 3 字节，÷1000
 * 通信：UDP socket，直连传感器
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"

#include <cstdint>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace smw;

class NBIT : public SensorBase {
public:
    NBIT(const std::string& name, const std::string& devnode);
    NBIT();
    ~NBIT() override;

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    int fd() const override { return sockfd_; }
    int ReadData() override;
    int WriteData() override;

private:
    void initCommands(uint8_t addr);

    int sockfd_;

    /* 传感器地址（从 INI 读取） */
    std::string ip_;
    int port_;

    /* 命令帧 */
    uint8_t cmdData_[5];

    /* 接收缓冲 */
    static const int kResponseLen = 23;
    uint8_t respBuf_[kResponseLen];

    /* 解析结果 */
    double sum_[6];
};

CLASS_LOADER_REGISTER_CLASS(NBIT, SensorBase)
