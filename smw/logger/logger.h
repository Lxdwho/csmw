/**
 * @brief 单例日志类
 * @date 2025.11.10
 */

#ifndef _SMW_LOGGER_LOGGER_H_
#define _SMW_LOGGER_LOGGER_H_

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include <errno.h>
#include <cstring>
#include <mutex>
// #include "logstream.h" // 循环依赖

namespace smw 	 {
namespace logger {

class LogStream;		// 使用声明防止循环依赖
class Logger {
public:
	enum Level {
		LOG_DEBUG = 0,
		LOG_INFO,
		LOG_WARN,
		LOG_ERROR,
		LOG_FATAL,
		LOG_COUNT
	};
	static Logger* Get_instance();
	void open(const std::string& filename);
	void close();
	void Set_maxstr(int bytes);
	void level(Level level);
	void Set_console(bool console);
	void log(Level level, const char* file, int line, const char* format, ...);
	void rotate();
	LogStream logStream(Level level, const char* file, int line);

private:
	Logger() // = default; 
	{
		// open("logt.txt");
		level(LOG_DEBUG);
		Set_console(true);
	}
	Logger(const Logger& log) = default;
	Logger& operator=(const Logger& log) = default;

	// 日志存储文件句柄
	std::ofstream m_fout;
	// 日志文件名称
	std::string log_file;
	// 日志文件字符数
	int file_len;
	// 日志文件最大字符数
	int max_filestr;
	// 推送日志等级
	Level log_level;
	// 控制台输出标志
	bool isconsole;
	// 
	static const char* s_level[LOG_COUNT];
	//
	std::mutex m_mutex;

};

} // namespace logger
} // namespace smw

#endif // _SMW_LOGGER_LOGGER_H_
