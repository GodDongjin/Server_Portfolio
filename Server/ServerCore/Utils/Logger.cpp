#include "pch.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "Logger.h"
#include "CorePch.h"

void Logger::init(std::string directory)
{
	string file_path = directory;

	auto now = chrono::system_clock::now();
	time_t now_time = chrono::system_clock::to_time_t(now);

	tm local_time;
	localtime_s(&local_time, &now_time);

	stringstream ss;
	ss << put_time(&local_time, "%Y_%m_%d_Log.txt");
	string file_name = ss.str();

	filesystem::path _directory("./Log");
	_full_path = _directory / file_name;

	if (!filesystem::exists(directory)) {
		filesystem::create_directories(directory);
	}

	ofstream file(_full_path, ios::app);

	if (!file) {
		cout << "颇老 积己 角菩\n";
		return;
	}

	cout << "颇老 积己 肯丰: " << _full_path << endl;
}

void Logger::shutdown()
{

}

void Logger::write(LogLevel level, const std::string& message)
{
	std::lock_guard<std::mutex> guard(_lock);

	ofstream file(_full_path, ios::app);

	if (!file) {
		cout << "颇老 积己 角菩\n";
		return;
	}

	auto now = chrono::system_clock::now();
	time_t now_time = chrono::system_clock::to_time_t(now);

	tm local_time;
	localtime_s(&local_time, &now_time);

	stringstream ss;
	ss << put_time(&local_time, "%H_%M_%S");
	string file_name = ss.str();

	switch (level)
	{
	case LogLevel::LOG_DEBUG:
		file << "[DEBUG]  [" + ss.str() + "]  " + message << endl;
		break;
	case LogLevel::LOG_INFO:
		file << "[INFO]  [" + ss.str() + "]  " + message << endl;
		break;
	case LogLevel::LOG_WRAN:
		file << "[WRAN]  [" + ss.str() + "]  " + message << endl;
		break;
	case LogLevel::LOG_ERROR:
		file << "[ERROR]  [" + ss.str() + "]  " + message << endl;
		break;
	default:
		break;
	}
}

//std::string Logger::make_log_line(LogLevel level, const char* file, int32 line, const string& message)
//{
//	return std::string();
//}

const char* Logger::level_to_string(LogLevel level)
{
	return nullptr;
}
