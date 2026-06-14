/**
 * @brief:速度传感器驱动（接入 SubLoop 模式）
 */

#pragma once

#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "../../../smw/logger/log.h"

using namespace smw;

#define SUDU_Buffer_Length 60

class BRT38 : public SensorBase
{
public:
    BRT38() : fd_(-1) {}
    ~BRT38() { if (fd_ >= 0) Release(); }

    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    /* SubLoop 接口 */
    int fd() const override { return fd_; }
    int ReadData() override;

    /* 返回ch字符在sign数组中的序号 */
    int getIndex0fSigns(char ch);
    /* 十六进制数转换为十进制数 */
    long hexToDec(char* source, int len);

private:
    void sendCommand();

    int fd_;
    //发送命令帧 + 接收缓冲区
    //该传感器使用： 串口发送一段命令过去，才会返回转速的数据，即下面这行命令
    //const char readcmd_[8] = { 0x01, 0x03, 0x00, 0x20, 0x00, 0x02, 0xC5, 0xC1};
    /* 测试命令 */
    const char readcmd_[8] = { 0x03, 0x04, '0', '0', '0', 'a','9', '8' };
    char buf_[SUDU_Buffer_Length];
};

/* 注册工厂 */
CLASS_LOADER_REGISTER_CLASS(BRT38, SensorBase)
