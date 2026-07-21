/**
 * @brief ATGM336H GPS 模块驱动
 * @date  2026.07.20
 *
 * 从 tmp/GPS/ATGM336H 标准化移植，适配 SensorBase + SubLoop 事件驱动架构
 * - 非阻塞读取，SubLoop poll 触发 ReadData()
 * - 增量缓冲，逐帧查找 $GNRMC/$GNGGA/$GNVTG
 * - 三句齐全后 pushData 一帧 GpsData
 */

#include "ATGM336H.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../../../smw/ini/ini_file.h"

using namespace smw::ini;

// ===================== 构造 / 析构 =====================

ATGM336H::ATGM336H(const std::string& name, const std::string& devnode)
    : SensorBase(name, devnode), baudRate_(9600),
      rawIdx_(0), hasRmc_(false), hasGga_(false), hasVtg_(false)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
    memset(gnrmc_, 0, sizeof(gnrmc_));
    memset(gngga_, 0, sizeof(gngga_));
    memset(gnvtg_, 0, sizeof(gnvtg_));
}

ATGM336H::ATGM336H()
    : baudRate_(9600),
      rawIdx_(0), hasRmc_(false), hasGga_(false), hasVtg_(false)
{
    memset(rawBuf_, 0, sizeof(rawBuf_));
    memset(gnrmc_, 0, sizeof(gnrmc_));
    memset(gngga_, 0, sizeof(gngga_));
    memset(gnvtg_, 0, sizeof(gnvtg_));
}

ATGM336H::~ATGM336H() {
    if (serial_.IsOpened()) Release();
}

// ===================== 生命周期 =====================

int ATGM336H::Init() {
    log_info("[GPS:%s] 打开设备 %s", name_.c_str(), devnode_.c_str());

    // 从 INI 读取波特率（可选，默认 9600）
    IniFile* ini = IniFile::Get_instance();
    if (ini && ini->has(name_, "baud_rate")) {
        baudRate_ = (int)ini->get(name_, "baud_rate");
    }

    int fd = serial_.Open(devnode_, baudRate_);
    if (fd < 0) {
        log_error("[GPS:%s] 打开失败", name_.c_str());
        status_ = SensorStatus::kError;
        return -1;
    }

    status_ = SensorStatus::kReady;
    log_info("[GPS:%s] 初始化成功 (fd=%d, baud=%d)", name_.c_str(), fd, baudRate_);
    return 0;
}

int ATGM336H::Start() {
    if (status_ != SensorStatus::kReady) {
        log_error("[GPS:%s] 状态不正确，无法启动", name_.c_str());
        return -1;
    }
    // 重置解析状态
    rawIdx_ = 0;
    hasRmc_ = hasGga_ = hasVtg_ = false;
    status_ = SensorStatus::kCapturing;
    log_info("[GPS:%s] 已启动采集 (fd=%d)", name_.c_str(), serial_.fd());
    return 0;
}

int ATGM336H::Stop() {
    status_ = SensorStatus::kReady;
    log_info("[GPS:%s] 已停止采集", name_.c_str());
    return 0;
}

int ATGM336H::Release() {
    serial_.Close();
    status_ = SensorStatus::kDefault;
    log_info("[GPS:%s] 已释放", name_.c_str());
    return 0;
}

// ===================== 数据读取（SubLoop 调用） =====================

int ATGM336H::ReadData() {
    if (!serial_.IsOpened()) return -1;

    // SubLoop poll 触发后读取，追加到 rawBuf_
    int n = serial_.Read(rawBuf_ + rawIdx_, kRawBufSize - rawIdx_ - 1);
    if (n < 0) {
        log_error("[GPS:%s] 读取错误", name_.c_str());
        if (on_error_) on_error_(name_, errno);
        status_ = SensorStatus::kError;
        return -1;
    }
    if (n == 0) return 0;

    rawIdx_ += n;
    rawBuf_[rawIdx_] = '\0';  // 便于 strstr 搜索

    // 在缓冲区中逐帧提取 NMEA 语句
    // 每次找到一个完整语句（$... 到下一个 $ 或缓冲区末尾之前），解析后前移
    int scanPos = 0;
    while (scanPos < rawIdx_) {
        // 查找 '$'
        char* dollar = strchr((char*)rawBuf_ + scanPos, '$');
        if (!dollar) {
            // 无有效起始符，丢弃已扫描部分
            break;
        }
        int startPos = (uint8_t*)dollar - rawBuf_;

        // 查找下一个 '$' 或 '\r'/'\n' 作为语句结束
        char* nextDollar = strchr(dollar + 1, '$');
        char* crlf = strstr(dollar + 1, "\r\n");
        if (!crlf) crlf = strstr(dollar + 1, "\n");

        char* endMark = nullptr;
        if (nextDollar && crlf) {
            endMark = (nextDollar < crlf) ? nextDollar : crlf;
        } else if (nextDollar) {
            endMark = nextDollar;
        } else if (crlf) {
            endMark = crlf;
        } else {
            // 语句不完整，等待更多数据
            break;
        }

        int endPos = (uint8_t*)endMark - rawBuf_;
        int sentenceLen = endPos - startPos;
        if (sentenceLen < 6 || sentenceLen >= 128) {
            scanPos = endPos;
            continue;
        }

        // 拷贝语句到临时缓冲
        char sentence[128];
        memcpy(sentence, rawBuf_ + startPos, sentenceLen);
        sentence[sentenceLen] = '\0';

        // 去除尾部 \r\n
        for (int i = sentenceLen - 1; i >= 0; i--) {
            if (sentence[i] == '\r' || sentence[i] == '\n') {
                sentence[i] = '\0';
            } else {
                break;
            }
        }

        // 识别并存储语句
        if (strstr(sentence, "RMC")) {
            strncpy(gnrmc_, sentence, sizeof(gnrmc_) - 1);
            hasRmc_ = true;
        } else if (strstr(sentence, "GGA")) {
            strncpy(gngga_, sentence, sizeof(gngga_) - 1);
            hasGga_ = true;
        } else if (strstr(sentence, "VTG")) {
            strncpy(gnvtg_, sentence, sizeof(gnvtg_) - 1);
            hasVtg_ = true;
        }

        // 推进扫描位置（跳过结束标记）
        if (*endMark == '$') {
            scanPos = endPos;  // 下一轮从这个 $ 开始
        } else {
            scanPos = endPos + (crlf ? 2 : 1);  // 跳过 \r\n
        }
    }

    // 前移未处理的尾部数据
    if (scanPos > 0 && scanPos < rawIdx_) {
        int remain = rawIdx_ - scanPos;
        memmove(rawBuf_, rawBuf_ + scanPos, remain);
        rawIdx_ = remain;
    } else if (scanPos >= rawIdx_) {
        rawIdx_ = 0;
    }
    rawBuf_[rawIdx_] = '\0';

    // 三句齐全，组装一帧 GpsData
    if (hasRmc_) {
        auto gps = std::make_shared<GpsData>();
        gps->devName = name_;
        gps->frequency = 1;

        parseGNRMC(gnrmc_, *gps);

        if (hasGga_) parseGNGGA(gngga_, *gps);
        if (hasVtg_) parseGNVTG(gnvtg_, *gps);

        pushData(gps);

        hasRmc_ = hasGga_ = hasVtg_ = false;
    }

    return 0;
}

// ===================== NMEA 解析 =====================

bool ATGM336H::extractField(const char* sentence, int index, char* buf, int bufSize) {
    const char* p = sentence;
    // 跳过到第 index 个逗号
    for (int i = 0; i < index; i++) {
        p = strchr(p, ',');
        if (!p) return false;
        p++;
    }
    // 拷贝到下一个逗号或 '*'（校验和前缀）
    const char* end = p;
    while (*end && *end != ',' && *end != '*') end++;
    int len = end - p;
    if (len <= 0 || len >= bufSize) return false;
    memcpy(buf, p, len);
    buf[len] = '\0';
    return true;
}

double ATGM336H::nmeaToDegrees(const char* nmea) {
    // NMEA 格式：ddmm.mmmm（纬度）或 dddmm.mmmm（经度）
    double raw = atof(nmea);
    int degrees = (int)(raw / 100.0);
    double minutes = raw - degrees * 100.0;
    return degrees + minutes / 60.0;
}

bool ATGM336H::parseGNRMC(const char* sentence, GpsData& out) {
    // $GNRMC,hhmmss.ss,A/V,ddmm.mmmm,N/S,dddmm.mmmm,E/W,spd,crs,date,...*cs
    //  注意：sentence 以 $xxRMC 开头，extractField 的 index 从逗号分隔的字段计数
    //  因此字段 0 = "$xxRMC"（tag），实际数据从字段 1 开始
    char buf[32];

    // 字段 1: UTC 时间
    if (extractField(sentence, 1, buf, sizeof(buf))) {
        out.UTCTime = buf;
    }

    // 字段 2: 有效性 A/V
    if (extractField(sentence, 2, buf, sizeof(buf))) {
        out.sentenceHasFix = (buf[0] == 'A');
    }

    // 字段 3: 纬度
    if (extractField(sentence, 3, buf, sizeof(buf))) {
        out.latitude = nmeaToDegrees(buf);
    }

    // 字段 4: N/S
    if (extractField(sentence, 4, buf, sizeof(buf))) {
        out.N_S = buf;
        if (buf[0] == 'S') out.latitude = -out.latitude;
    }

    // 字段 5: 经度
    if (extractField(sentence, 5, buf, sizeof(buf))) {
        out.longitude = nmeaToDegrees(buf);
    }

    // 字段 6: E/W
    if (extractField(sentence, 6, buf, sizeof(buf))) {
        out.E_W = buf;
        if (buf[0] == 'W') out.longitude = -out.longitude;
    }

    // 字段 7: 地面速率（节）→ km/h
    if (extractField(sentence, 7, buf, sizeof(buf))) {
        out.speed = atof(buf) * 1.852;
    }

    // 字段 8: 地面航向（度）
    if (extractField(sentence, 8, buf, sizeof(buf))) {
        out.course = atof(buf);
    }

    return true;
}

bool ATGM336H::parseGNGGA(const char* sentence, GpsData& out) {
    // $GNGGA,hhmmss.ss,lat,N,lon,E,qual,sat,hdop,alt,M,...*cs
    //  字段:  0       1    2  3  4  5  6    7    8    9   10
    //  field 0 = tag "$GNGGA"
    char buf[32];

    // 字段 7: 卫星数量
    if (extractField(sentence, 7, buf, sizeof(buf))) {
        out.satellites = atoi(buf);
    }

    // 字段 8: HDOP
    if (extractField(sentence, 8, buf, sizeof(buf))) {
        out.hdop = atof(buf);
    }

    // 字段 9: 海拔
    if (extractField(sentence, 9, buf, sizeof(buf))) {
        out.altitude = atof(buf);
    }

    return true;
}

bool ATGM336H::parseGNVTG(const char* sentence, GpsData& out) {
    // $GNVTG,crs,T,crs,M,spd,N,spd,K,mode*cs
    //  字段:  0    1  2  3  4  5  6  7  8
    //  field 0 = tag "$GNVTG"
    char buf[32];

    // 字段 1: 地面航向（真北，度）
    if (extractField(sentence, 1, buf, sizeof(buf))) {
        out.course = atof(buf);
    }

    // 字段 7: 地面速率（km/h）
    if (extractField(sentence, 7, buf, sizeof(buf))) {
        out.speed = atof(buf);
    }

    return true;
}
