/**
 * @brief 流式写入日志
 * @date 2025.11.10
 */

#ifndef _SMW_LOGGER_LOGSTREAM_H_
#define _SMW_LOGGER_LOGSTREAM_H_

#include <sstream>
#include "logger.h"

namespace smw    {
namespace logger {

using namespace smw::logger;

class LogStream {
public:
    LogStream(Logger::Level level, const char* file, int line)
            : m_level(level), m_file(file), m_line(line) {}
    ~LogStream() {
        Logger::Get_instance()->log(m_level, m_file, m_line, "%s", m_stream.str().c_str());
    }
    
    template<typename T>
    LogStream& operator<<(const T& msg) {
        m_stream << msg;
        return *this;
    }

    LogStream(const LogStream& other) : 
        m_level(other.m_level), m_file(other.m_file), m_stream(other.m_stream.str()) {
        std::cout << "debug logstream!" << std::endl;
    }

private:
    Logger::Level m_level;
    const char* m_file;
    int m_line;
    std::ostringstream m_stream;
};

} // namespace logger
} // namespace smw

#endif // _SMW_LOGGER_LOGSTREAM_H_
