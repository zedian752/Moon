#pragma once
#include <cstdarg>
#include <string>
#include "iostream"
class Log
{
public:
	enum class log_level :char { NOLOG, DEBUG, INFO, WARNING, ERROR };

	static Log* get_instance();

	void log(const char* file, int line_number, log_level log_level, const char* fmt, ...);

private:
	//static Log* sec_log_instance;
	std::string error_getcurtime(void);
	int error_openfile(int* pf);
	void error_core(const char* file, int line_number, int log_level, const char* fmt, va_list args);
};

