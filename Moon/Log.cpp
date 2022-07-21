#define _CRT_SECURE_NO_WARNINGS
#include "Log.h"
#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>

#ifdef __GNUC__
#include <unistd.h>
#endif

#ifdef WIN32
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//Log * Log::sec_log_instance = new Log();
static constexpr int LOG_MAX_STRING_LEN = 10240;
/* 输入到本地的持久化文件名*/
static constexpr const char* LOG_FILE_NAME = "debug_log.log";
// NOLOG:代表是否需要写入log文件中
const std::string level_name[5] = { "NOLOG", "DEBUG", "INFO", "WARNING", "ERROR"};



void Log::log(const char* file, int line_number, Log::log_level log_level, const char* fmt, ...)
{
	if ((int)log_level == (int)Log::log_level::NOLOG) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	error_core(file, line_number, static_cast<int>(log_level), fmt, args);
	va_end(args);
}

Log* Log::get_instance()
{
	static Log* sec_log_instance;
	return sec_log_instance;
}

void Log::error_core(const char* file_name, int line_number, int log_level, const char* fmt, va_list args)
{
	std::stringstream ss;

	// 写入打日志时间
	std::string format_time = error_getcurtime();
	ss << "[" << format_time << "]";

	// 写入日志级别
	ss << "[" << level_name[log_level] << "]";

	//// 写入log状态
	//if (status != 0) {
	//	ss << "[ERROR is " << status << "]";
	//}
	/*else {
		ss << "[SUCCESS]";
	}*/
	char log_info[LOG_MAX_STRING_LEN] = { 0x0 };
	vsnprintf(log_info, LOG_MAX_STRING_LEN, fmt, args);
	ss << std::string(log_info); // 接受了一个右值


	// 写入日志发生文件
	ss << "[" << file_name << "]";

	// 写入日志发生文件行数
	ss << "[" << line_number << "]\n";

/* 控制日志是否持久化的marco */
#ifdef SEC_LOG:LOG_TOFILE

	int fd = 0;
	error_openfile(&fd);
	if (fd > 0) {
		write(fd, ss.str().data() ,ss.str().size());
	}
	close(fd);
#else

	printf("%s\n", ss.str().data());

#endif
}

/*
	获取格式化字符串% F 代表 年 - 月 - 日 的组合。 % T 代表 时：分：秒的组合
*/


std::string Log::error_getcurtime()
{
	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::stringstream ss;
	auto c  = std::put_time(std::localtime(&t), "%F %T");
	ss << c;
	return ss.str();
}

// 检查目录是否存在，如果不存在则建立
int check_and_create_dir(const char * path_name) {
#ifdef __GNUC__
	int is_exist = access(path_name, F_OK);
	if (is_exist == 0) {
		return is_exist;
	}
	// 只能创建1级目录
	int status = mkdir(path_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // RWX-USER RWX-GROUP R-OTH X-OTH
	if (status != 0) {
		perror("创建文件夹失败");
	}
	return status;
#else
	return -1;
#endif // !__GNUC__
}

int Log::error_openfile(int* pf)
{
#ifdef __GNUC__
	char file_name[1024] = { 0x0 };
	sprintf(file_name, "./%s", LOG_FILE_NAME);

	constexpr const char * path_name = "./log/";
	sprintf(file_name, "%s/%s", path_name, LOG_FILE_NAME);
	int status = check_and_create_dir(path_name);
	if (status != 0) {
		return -1; // 创建目录失败
	}
	*pf = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (*pf < 0) {
		perror("打开文件失败原因：");
		return -1;
	}
#endif // __GNUC__
	return 0;
}
