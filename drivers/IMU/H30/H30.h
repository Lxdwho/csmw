/**
 * @brief H30/H30mini驱动
 * @date 2026.06.14
 */

#pragma once 
#include <string>
#include "../../../smw/Sensor_driver.h"
#include "../../../smw/classFactory/ClassFactory.h"
#include "analysis_data.h"

using namespace smw;

#define BUFLEN 512

class H30 : public SensorBase {
    public:
    H30(const std::string& name, const std::string& devnode);
    H30();
    ~H30() override;

    // 生命周期
    int Init() override;
    int Start() override;
    int Stop() override;
    int Release() override;

    /* SubLoop 接口 */
    int fd() const override { return fd_; }
    int ReadData() override;  // 非阻塞读取，触发 on_data_ 回调

    /* 数据解析方法 */

private:
    int fd_;
    char buffer_[BUFLEN];
    unsigned short g_recv_buf_idx = 0;
    unsigned char g_recv_buf[BUFLEN];
    protocol_info_t g_output_info = {0};
};

CLASS_LOADER_REGISTER_CLASS(H30, SensorBase)
