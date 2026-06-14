/**
 * @brief 单例日志类实现
 * @date 2025.11.10
 */

#include "logger.h"
#include "logstream.h"

namespace smw    {
namespace logger {

Logger* Logger::Get_instance() {
	static Logger log; // static 和new可以一起使用吗？
	return &log;
}

// 打开日志文件
void Logger::open(const std::string& filename) {
	if (m_fout.is_open()) {
        m_fout.close();  // 先安全关闭
    }
	log_file = filename;
	m_fout.open(log_file.c_str(), std::ios::app);
	if (m_fout.fail()) {
		throw std::logic_error("open log file failed: " + log_file);
	}
	m_fout.seekp(0, std::ios::end);
	file_len = m_fout.tellp();
}

// 关闭日志文件
void Logger::close() {
	m_fout.close();
}

// 设置日志文件的最大字符容量
void Logger::Set_maxstr(int bytes) {
	max_filestr = bytes;
}

// 设置日志输出等级
void Logger::level(Level level) {
	log_level = level;
}

// 设置控制台输出标志
void Logger::Set_console(bool console) {
	isconsole = console;
}

const char* Logger::s_level[LOG_COUNT] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};

// 日志执行函数
void Logger::log(Level level, const char* file, int line, const char* format, ...) {
	if (log_level > level) return;
	if (m_fout.fail()) return;

	time_t ticks = time(NULL);
	struct tm* ptm = localtime(&ticks);

	// 获取当前时间
	char timestamp[32];
	memset(timestamp, 0, sizeof(timestamp));
	strftime(timestamp, sizeof(timestamp), "%y-%m-%d %H:%M:%S", ptm);

	// 
	std::ostringstream oss;
	const char* fmt = "%s %s %s:%d ";
	int len = snprintf(NULL, 0, fmt, timestamp, s_level[level], file, line);
	if (len > 0) {
		char* buffer = new char[len + 1];
		snprintf(buffer, len + 1, fmt, timestamp, s_level[level], file, line);
		buffer[len] = 0;
		oss << buffer;
		delete [] buffer;
		file_len += len;
	}

	// 
	va_list arg_ptr;
	va_start(arg_ptr, format);
	len = vsnprintf(NULL, 0, format, arg_ptr);
	va_end(arg_ptr);
	if (len > 0) {
		char* content = new char[len + 1];
		va_start(arg_ptr, format);
		vsnprintf(content, len + 1, format, arg_ptr);
		va_end(arg_ptr);
		content[len] = 0;
		oss << content;
		delete[] content;
		file_len += len;
	}

	oss << '\n';
	const std::string& str = oss.str();
	if (isconsole) {
		std::cout << str; // << std::endl;
	}
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_fout << str;
		m_fout.flush();
	}
	if (max_filestr > 0 && file_len >= max_filestr) rotate();
}

void Logger::rotate() {
	close();
	time_t ticks = time(NULL);
	struct tm* ptm = localtime(&ticks);
	char timestamp[32];
	memset(timestamp, 0, sizeof(timestamp));
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", ptm);
	std::string filename = log_file + timestamp;
	if (rename(log_file.c_str(), filename.c_str()) != 0) {
		throw std::logic_error("rename log file faild: " + std::string(strerror(errno)));
	}
	open(log_file);
}

LogStream Logger::logStream(Level level, const char* file, int line) {
    return LogStream(level, file, line);
}

} // namespace logger
} // namespace smw
