/**
 * @brief H30/H30mini驱动
 * @date 2026.06.14
 */

#include "H30.h"
#include "../../../smw/logger/log.h"
#include <iostream>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

H30::H30(const std::string& name, const std::string& devnode)
        : SensorBase(name, devnode), fd_(-1) { }
H30::H30() : fd_(-1) { }
H30::~H30() { Release(); }

// 生命周期
int H30::Init() {
    log_info("[Serial:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());
    fd_ = open(devnode_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ == -1) {
        log_error("[Serial:%s] 打开失败: %s", name_.c_str(), strerror(errno));
        status_ = SensorStatus::kError;
        return -1;
    }

    // 2. 配置 termios（8N1，阻塞，无回显）
    struct termios newtio;
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = B460800 | CS8 | CLOCAL | CREAD;
    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cflag &= ~PARENB;

    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 关键！

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 1; // 至少 1 字节才返回

    tcflush(fd_, TCIOFLUSH);
    tcsetattr(fd_, TCSAFLUSH, &newtio);

    status_ = SensorStatus::kReady;
    log_info("[Serial:%s] 初始化成功 (fd=%d)", name_.c_str(), fd_);
    return 0;
}

int H30::Start() {
    if(status_ != SensorStatus::kReady) {
        log_error("[Serial:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    status_ = SensorStatus::kCapturing;
    log_info("[Serial:%s] 已启动采集 (fd=%d)", name_.c_str(), fd_);
    return 0;
}

int H30::Stop() {
    status_  = SensorStatus::kReady;
    log_info("[Serial:%s] 已停止采集", name_.c_str());
    return 0;
}

int H30::Release() {
    if(fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    status_ = SensorStatus::kDefault;
    log_info("[Serial:%s] 已释放", name_.c_str());
    return 0;
}

/* SubLoop 接口 */
int H30::ReadData() {
    ProtocolAnalysis proana;
    uint16_t cnt = 0;
    int pos = 0;
    int nread = read(fd_, buffer_, BUFLEN);

    if(nread > 0) {
        memcpy(g_recv_buf + g_recv_buf_idx, buffer_, nread);             
        g_recv_buf_idx += nread;
    }
    else if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  // 没有数据，正常返回
        }
        log_error("[Serial:%s] 读取错误: %s", name_.c_str(), strerror(errno));
        if (on_error_) {
            on_error_(name_, errno);
        }
        status_ = SensorStatus::kError;
        return -1;
    }

    cnt = g_recv_buf_idx;
    pos = 0;
    if(cnt < YIS_OUTPUT_MIN_BYTES) {
        return 0;
    }

    while(cnt > 0) {
        int ret = proana.analysisData(g_recv_buf + pos, cnt, &g_output_info);
        if(analysis_done == ret) {
            pos++;
            cnt--;
        }
        else if(data_len_err == ret) {
            break;
        }
        else if(crc_err == ret || analysis_ok == ret) {
            output_data_header_t *header = (output_data_header_t *)(g_recv_buf + pos);
            unsigned int frame_len = header->len + YIS_OUTPUT_MIN_BYTES;
            cnt -= frame_len;
            pos += frame_len;
            //memcpy(g_recv_buf, g_recv_buf + pos, cnt);

            if(analysis_ok == ret) {
                auto H30_data = std::make_shared<ImuData>();
                H30_data->devName = "H30";
                H30_data->frequency  = 10;
                //Imu data
                H30_data->magnetometer_x = g_output_info.raw_mag.x;        
                H30_data->magnetometer_y = g_output_info.raw_mag.y;        
                H30_data->magnetometer_z = g_output_info.raw_mag.z; 
                H30_data->roll = g_output_info.attitude.pitch;
                H30_data->pitch = g_output_info.attitude.roll;
                H30_data->yaw =  g_output_info.attitude.yaw;
                // log_debug("roll: %.4f\t pitch: %.4f\t yaw: %.4f", 
                //            H30_data->roll, H30_data->pitch, H30_data->yaw);
                pushData(H30_data);
            }
            else 
                log_debug("pares error");
	    }
	}
    memcpy(g_recv_buf, g_recv_buf + pos, cnt);
    g_recv_buf_idx = cnt;
    tcflush(fd_,TCIFLUSH);
    return 0;
}
