#include <errno.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <linux/tty_flags.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
#include "../logger/log.h"

#define FIFO_SIZE 64
class ttySerial {
 public:
  ttySerial() { serial_port_ = -1; }

  int Open(const std::string& serial_name, int baud_rate) {
    serial_port_ = open(serial_name.c_str(), O_RDWR | O_NOCTTY);
    AINFO << "serial_port_:" << serial_port_;
    if (serial_port_ < 0) {
      return -1;
    }

    AINFO << "Serial port open: " << serial_name
             << " Serial baud: " << baud_rate;

    if(LockSet(serial_port_, F_WRLCK) != 0) {
        AFATAL << "Get File Lock faild!";
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serial_port_, &tty) != 0) {
      AERROR << "Error " << errno << " from tcgetattr " << errno;
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 1;

    switch (baud_rate) {
      case 4800:
        baud_rate = B4800;
        break;
      case 9600:
        baud_rate = B9600;
        break;
      case 19200:
        baud_rate = B19200;
        break;
      case 38400:
        baud_rate = B38400;
        break;
      case 57600:
        baud_rate = B57600;
        break;
      case 115200:
        baud_rate = B115200;
        break;
      case 230400:
        baud_rate = B230400;
        break;
      case 460800:
        baud_rate = B460800;
        break;
      case 576000:
        baud_rate = B576000;
        break;
      case 921600:
        baud_rate = B921600;
        break;
      default:
        AERROR << "unknown baud rate:" << baud_rate
                  << " add code to support";
        return -1;
    }
    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    if (tcsetattr(serial_port_, TCSANOW, &tty) != 0) {
      const char* error_string = strerror(errno);
      AERROR << "Error " << errno << "from tcsetattr: " << error_string;
    }

    struct serial_struct serial;
    if (ioctl(serial_port_, TIOCGSERIAL, &serial) == 0) {
      serial.flags |= ASYNC_LOW_LATENCY;
      serial.xmit_fifo_size = FIFO_SIZE;
      if (ioctl(serial_port_, TIOCSSERIAL, &serial) < 0) {
        AWARN << "ASYNC_LOW_LATENCY not supported (USB-serial?), ignored: "
              << strerror(errno);
      }
    }

    tcflush(serial_port_, TCIOFLUSH);

    return serial_port_;
  }

  bool IsOpened() { return serial_port_ >= 0; }
  int fd() const { return serial_port_; }

  int LockSet(int fd, int type) {
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = type;
    lock.l_pid = -1;

    if (fcntl(fd, F_GETLK, &lock) < 0) {
      const char* error_string = strerror(errno);
      AERROR << "F_GETLK, error code: " << error_string;
      return -1;
    }

    if (lock.l_type != F_UNLCK) {
      if (lock.l_type == F_RDLCK) {
        AERROR << "Read lock already set by " << lock.l_pid;
        return -1;

      } else if (lock.l_type == F_WRLCK) {
        AERROR << "Write lock already set by " << lock.l_pid;
        return -1;
      }
    }

    lock.l_type = type;

    if ((fcntl(fd, F_SETLK, &lock)) < 0) {
      AERROR << "Lock failed : type = " << lock.l_type;
      return -2;
    }

    switch (lock.l_type) {
      case F_RDLCK: {
        AINFO << "Read lock set by " << getpid();
        return 0;
      } break;
      case F_WRLCK: {
        AINFO << "write lock set by " << getpid();
        return 0;
      } break;
      case F_UNLCK: {
        AINFO << "Release lock by ", getpid();
        return 0;
      } break;

      default:
        break;
    }
    return 0;
  }

  /**
   * @brief 串口读取（由 SubLoop poll 触发，fd 已确认可读）
   * @param buffer     接收缓冲区
   * @param max_length 缓冲区大小
   * @return >0 实际读取字节数，0 对端关闭，-1 错误
   */
  int Read(uint8_t* buffer, int max_length) {
    if (serial_port_ < 0) return -1;

    int num_bytes = read(serial_port_, buffer, max_length);
    if (num_bytes < 0) {
      AERROR << "read error: " << strerror(errno);
      return -1;
    }
    return num_bytes;
  }

  int Write(const uint8_t* buffer, int length) {
    int num_bytes = write(serial_port_, buffer, length);
    if (num_bytes < length) {
      const char* error_string = strerror(errno);
      AERROR << "error writing: " << error_string;
    }
    return num_bytes;
  }

  int Close() {
    if (serial_port_ >= 0) {
      LockSet(serial_port_, F_UNLCK);
      close(serial_port_);
      serial_port_ = -1;
    }
    return 0;
  }

 private:
  int serial_port_;
};

