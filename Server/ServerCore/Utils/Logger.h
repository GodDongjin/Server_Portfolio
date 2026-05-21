#pragma once

#include <filesystem>
#include <mutex>
#include <string>

enum class LogLevel
{
	LOG_DEBUG,
	LOG_INFO,
	LOG_WRAN,
	LOG_ERROR,
};

class Logger
{
public:
	void init(std::string directorys);
	void shutdown();

	void write(LogLevel level, const std::string& message);

private:
	const char* level_to_string(LogLevel level);

private:
	std::mutex _lock;
	std::filesystem::path _full_path;
};