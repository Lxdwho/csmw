/**
 * @brief: 所有传感器数据统一继承DataBase,形成统一数据接口
 * 都有公共属性,后续可以统一处理各种数据 DataBase* data
 * @date 2026.06.13
 */

#ifndef _SMW_DATABASE_H_
#define _SMW_DATABASE_H_

#include <cfloat>
#include <string>
#include <cstdint>
#include <vector>

namespace smw {

/* 定义一个结构体_DataBase, 并起别名DataBase */
typedef struct _DataBase {
    virtual ~_DataBase() = default;
    // int dataType = 0;           // 数据实际类型
    u_int32_t frequency = 0;    // 采集频率
    u_int32_t frameId = 0;      // 帧号ID
    uint64_t timestamp = 0;     // 时间戳ms
    std::string devName;        // 哪个设备来的数据,初始化的时候配置
} DataBase;
 
struct ImuData : public DataBase {
   float gyroscope_x;           // 机体系X轴角速度
   float gyroscope_y;           // 机体系Y轴角速度
   float gyroscope_z;           // 机体系Z轴角速度
   float accelerometer_x;       // 机体系X轴加速度(未分离重力加速度)
   float accelerometer_y;       // 机体系Y轴加速度(未分离重力加速度)
   float accelerometer_z;       // 机体系Z轴加速度(未分离重力加速度)
   float magnetometer_x;        // 机体系X轴磁感应强度
   float magnetometer_y;        // 机体系Y轴磁感应强度
   float magnetometer_z;        // 机体系Z轴磁感应强度
   float roll;                     
   float pitch;
   float yaw;
};

struct LidarScan :public DataBase 
{
    std::string frame_id;
    _Float32 angle_min;
    _Float32 angle_max;
    _Float32 angle_increment;
    _Float32 time_increment;
    _Float32 scan_time;
    _Float32 range_min;
    _Float32 range_max;
    std::vector<_Float32> ranges;
    std::vector<_Float32> intensities; 
};

struct ImageData : public DataBase{
    uint32_t channelId = 0;
    uint32_t channelNum = 0;
    // uint32_t format = 0;  // 参考 ImageFormatType
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 0;
    void *data = nullptr;
};

struct DepthData {
    uint32_t channelNum = 0;
    // uint32_t format = 0; 
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 0;
    void *data = nullptr;
};

struct PosData {
    float translation_x;
    float translation_y;
    float translation_z;
    float velocity_x;
    float velocity_y;
    float velocity_z;
};

struct D435ComposeFrame : public DataBase {
    ImageData *m_color_frame = nullptr;
    DepthData *m_depth_frame = nullptr;
};

struct T265ComposeFrame : public DataBase {
    T265ComposeFrame() {
        image_left = new ImageData();
        image_right = new ImageData();
        pose_data = new PosData();
        imu_data = new ImuData();
    }
    ImageData *image_left;
    ImageData *image_right;
    PosData   *pose_data;
    ImuData   *imu_data;
};

/* 六维力数据结构定义 */
struct PowerData : public DataBase
{
    float fx;//x轴力
    float fy;//y轴力
    float fz;//z轴力
    float Mx;//x轴力矩
    float My;//y轴力矩
    float Mz;//z轴力矩
};

/* GPS数据结构定义 */
struct GpsData : public DataBase
{
   uint64_t timestamp;
   _Float64  latitude;   //纬度（度）
   _Float64  longitude;  //经度（度）
   _Float64  altitude;   //海拔（米）
    bool   sentenceHasFix; // 定位是否有效
    size_t satellites;  //卫星数量
   _Float64 hdop;       //精度因子
   _Float64 speed;      //水平速率（km/h）
   _Float64 course;     //地面航向（度）
   std::string UTCTime; //UTC时间 hhmmss.ss
   std::string N_S;     //北纬/南纬 N/S
   std::string E_W;     //东经/西经 E/W
};

/* 速度传感器数据结构定义 */
struct SpeedData : public DataBase
{
   _Float64 n;     //转速
   _Float64 angle; // 单圈角度
};

} // namespace smw

#endif // _SMW_DATABASE_H_
