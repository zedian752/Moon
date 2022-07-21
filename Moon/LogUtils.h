#pragma once
#include "Log.h"

static auto Log = Log::get_instance();
#define log_debug(format, ...) Log->log(__FILE__, __LINE__, Log::log_level::DEBUG, format, ##__VA_ARGS__);
#define log_info(format, ...) Log->log(__FILE__, __LINE__, Log::log_level::INFO, format, ##__VA_ARGS__);
#define log_warning(format, ...) Log->log(__FILE__, __LINE__, Log::log_level::WARNING, format, ##__VA_ARGS__);
#define log_error(format, ...) Log->log(__FILE__, __LINE__, Log::log_level::ERROR, format, ##__VA_ARGS__);
