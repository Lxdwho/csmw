/**
 * @brief:速度传感器驱动（请求-响应型，定时写调度）
 *
 * 协议：发命令 → 收 4 字节十六进制转速 → 解析 → pushData
 * 由 MainLoop timerfd 定时触发 WriteData()，POLLIN 触发 ReadData()
 */

#include "BRT38.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

/* 返回ch字符在sign数组中的序号 */
int BRT38::getIndex0fSigns(char ch)
{
    if(ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    else if ( ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    else if ( ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

/* 十六进制数转换为十进制数 */
long BRT38::hexToDec(char* source, int len) {
    long sum = 0;
    long t = 1;
    for ( int i = len - 1; i >= 0; i--) {
        sum += t * getIndex0fSigns(source[i]);
        t *= 16;
    }
    return sum;
}

int BRT38::Init() {
    log_info("[Speed:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    int fd = serial_.Open(devnode_, 9600);
    if (fd < 0) {
        log_error("[Speed:%s] 打开失败", name_.c_str());
        status_ = SensorStatus::kError;
        return -1;
    }

    /* 从 INI 读取写间隔（默认 50ms） */
    int writeMs = 50;
    IniFile* ini = IniFile::Get_instance();
    if (ini->has(name_, "write_interval")) {
        writeMs = (int)ini->get(name_, "write_interval");
    }
    setWriteInterval(writeMs);

    status_ = SensorStatus::kReady;
    log_info("[Speed:%s] 初始化成功 (fd=%d)", name_.c_str(), fd);
    return 0;
}

int BRT38::Start() {
    if(status_ != SensorStatus::kReady) {
        log_error("[Speed:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }

    status_ = SensorStatus::kCapturing;
    /* 请求第一次写机会（由 MainLoop timerfd 调度） */
    requestWrite();
    log_info("[Speed:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int BRT38::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[Speed:%s] 已停止采集", name_.c_str());
    return 0;
}

int BRT38::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[Speed:%s] 已释放", name_.c_str());
    return 0;
}

/* 由 MainLoop timerfd 通过 queueInLoop 调度 */
int BRT38::WriteData() {
    int ret = serial_.Write(reinterpret_cast<const uint8_t*>(readcmd_), 8);
    if (ret < 0) {
        log_error("[Speed:%s] 发送命令失败", name_.c_str());
    }
    markWriteDone();  // 清 writeRequested + 记时间戳
    return 0;
}

/* 由 SubLoop POLLIN 触发 */
int BRT38::ReadData() {
    if (!serial_.IsOpened()) return -1;

    bzero(buf_, SUDU_Buffer_Length);
    int ret = serial_.Read(reinterpret_cast<uint8_t*>(buf_), 10);

    if (ret > 0) {
        int i = 0;
        for(; i < ret - 1; i++) {
            /* 找到数据头 0x03 0x04，跳到数据区 */
            if(buf_[i] == 0x03 && buf_[i+1] == 0x04) {
                i += 2;
                break;
            }
        }

        /* 解析 4 字节十六进制转速 */
        auto tmp = std::make_shared<SpeedData>();
        char hex[5];
        strncpy(hex, buf_ + i, 4);
        hex[4] = '\0';
        tmp->n = hexToDec(hex, 4);
        tmp->frameId = 0;
        tmp->frequency = 10;
        /* 编码器值 → 实际位移 */
        tmp->n = (float)tmp->n * 0.00915f;

        pushData(tmp);

        /* 请求下一次写（间隔由 setWriteInterval 控制） */
        requestWrite();
        return 0;
    }
    else if (ret < 0) {
        log_error("[Speed:%s] 读取错误", name_.c_str());
        if (on_error_) on_error_(name_, errno);
        status_ = SensorStatus::kError;
        return -1;
    }
    return 0;
}
