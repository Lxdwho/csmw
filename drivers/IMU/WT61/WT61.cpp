/**
 * 实现的是驱动的撰写?意味不明
 * 最后read 的fd与返回值操作不明确
 */

#include "WT61.h"

#include <fcntl.h>  /* 文件控制 */
#include <unistd.h> /* unix标准接口*/
#include <termios.h>/* 串口配置 */
#include <cstdio>
#include <cerrno>
#include <cstring>

/* name在manager里面设置,(setname函数)由main函数填入entry后设置*/
WT61::WT61(const std::string& name, const std::string& devnode)
    : SensorBase(name,devnode), fd_(-1) { }
WT61::WT61() : fd_(-1) { }

WT61:: ~WT61() {
    if(fd_ >= 0) Release();
 }

/**
 * @brief:初始化open→配置termios→设置波特率→应用配置tcsetattr
 */
int WT61::Init(){
    /* devnode_这个设备在linux下生成的文件的路径/dev/ttyUSB0 */
    /* 可以认为devnode_才是真正的Linux设备名*/
    /* 由manager函数简体后从INFO传入devnode*/
    log_info("\n[Serial:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());
    fd_ = open(devnode_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd_ < 0)
    {
        log_error("[Serial:%s] 打开失败: %s", name_.c_str(), strerror(errno));
        status_ = SensorStatus::kError;
        return -1;
    }

    /* 配置串口 115200 8N1 */
    struct termios tty {};
    /* 获取当前串口设备的配置参数 */
    tcgetattr(fd_, &tty);
    /* 设置输出波特率 */
    cfsetospeed(&tty, B115200);
    /* 设置输入波特率 */
    cfsetispeed(&tty, B115200);
    tty.c_cflag = CS8 | CLOCAL | CREAD;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    /* 设置属性 */
    tcsetattr(fd_, TCSANOW, &tty);

    status_ = SensorStatus::kReady;
    log_info("\n[Serial:%s] 初始化成功 (fd=%d)", name_.c_str(), fd_);
    return 0;
}

/* 串口设备切换到采集状态（不创建线程，由 SubLoop 管理）*/
int WT61::Start(){
    if(status_ != SensorStatus::kReady)
    {
        log_error("[Serial:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    status_ = SensorStatus::kCapturing;
    log_info("\n[Serial:%s] 已启动采集 (fd=%d)", name_.c_str(), fd_);
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

/* 非阻塞读取数据（由 SubLoop 的 select 调用）*/
int WT61::ReadData() {
    if (fd_ < 0) return -1;

    char buf[1024];
    ssize_t n = ::read(fd_, buf, sizeof(buf));

    if (n > 0) {
        // 解析数据（这里简化处理，实际应该解析协议）
        auto tmp = std::make_shared<ImuData>();
        tmp->frameId = 1;
        tmp->pitch = 0;
        tmp->yaw = 1;
        tmp->roll = 2;

        // 推拉结合：触发回调（推）+ 更新缓冲区（拉）
        pushData(tmp);
        return 0;
    }
    else if (n < 0) {
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
    return 0;
}