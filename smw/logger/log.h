/**
 * @brief 日志宏
 * @date 2025.11.10
 */

#ifndef _SMW_logger_LOG_H_
#define _SMW_logger_LOG_H_

#include "logger.h"
#include "logstream.h"

using namespace smw::logger;

#define Logger_Init(file_name) Logger::Get_instance()->open(file_name)
#define ADEBUG 	Logger::Get_instance()->logStream(Logger::LOG_DEBUG,__FILE__, __LINE__)
#define AINFO 	Logger::Get_instance()->logStream(Logger::LOG_INFO ,__FILE__, __LINE__)
#define AWARN 	Logger::Get_instance()->logStream(Logger::LOG_WARN ,__FILE__, __LINE__)
#define AERROR 	Logger::Get_instance()->logStream(Logger::LOG_ERROR,__FILE__, __LINE__)
#define AFATAL 	Logger::Get_instance()->logStream(Logger::LOG_FATAL,__FILE__, __LINE__)

#define log_debug(format, ...) Logger::Get_instance()->log(Logger::LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_info(format, ...)  Logger::Get_instance()->log(Logger::LOG_INFO , __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_warn(format, ...)  Logger::Get_instance()->log(Logger::LOG_WARN , __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_error(format, ...) Logger::Get_instance()->log(Logger::LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_fatal(format, ...) Logger::Get_instance()->log(Logger::LOG_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if !defined(RETURN_IF_NULL)
#define RETURN_IF_NULL(ptr)          \
  if (ptr == nullptr) {              \
    AWARN << #ptr << " is nullptr."; \
    return;                          \
  }
#endif


#if !defined(RETURN_VAL_IF_NULL)
#define RETURN_VAL_IF_NULL(ptr, val) \
  if (ptr == nullptr) {              \
    AWARN << #ptr << " is nullptr."; \
    return val;                      \
  }
#endif

#if !defined(RETURN_IF)
#define RETURN_IF(condition)           \
  if (condition) {                     \
    AWARN << #condition << " is met."; \
    return;                            \
  }
#endif

#if !defined(RETURN_VAL_IF)
#define RETURN_VAL_IF(condition, val)  \
  if (condition) {                     \
    AWARN << #condition << " is met."; \
    return val;                        \
  }
#endif

#endif // _SMW_logger_LOG_H_
