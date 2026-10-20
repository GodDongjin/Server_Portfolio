#pragma once
#include "ConfigReader.h"
#include <limits>

struct ClientConfig
{
	wstring server_ip;
	uint16 port = 0;
	int32 session_count = 0;
	int32 worker_thread_count = 0;
	int32 bot_thread_count = 0;
	int32 chat_interval_ms = 0;
	int32 bot_room_count = 0;
	int32 all_chat_interval = 0;
	int32 whisper_interval = 0;

	bool load(const ConfigReader& config, bool load_test)
	{
		server_ip = config.get_wstring("server", "ip");
		int32 port_value = 0;
		if (server_ip.empty() || !read_range(config, "server", "port", port_value, 1, 65535)) {
			return false;
		}
		// 범위를 확인한 다음 변환해야 음수나 65535 초과 값이 다른 포트로 바뀌지 않는다.
		port = static_cast<uint16>(port_value);
		if (!load_test) {
			return true;
		}
		// 큰 수의 설정 실수로 과도한 스레드와 버퍼를 만들지 않도록 상한도 둔다.
		return read_range(config, "client", "session_count", session_count, 1, 100000)
			&& read_range(config, "client", "worker_thread_count", worker_thread_count, 1, 256)
			&& read_range(config, "client", "bot_thread_count", bot_thread_count, 1, 256)
			&& read_range(config, "client", "chat_interval_ms", chat_interval_ms, 1, 60000)
			&& read_range(config, "client", "bot_room_count", bot_room_count, 1, 256, 10)
			&& read_range(config, "client", "all_chat_interval", all_chat_interval,
				1, (numeric_limits<int32>::max)(), 10)
			&& read_range(config, "client", "whisper_interval", whisper_interval,
				1, (numeric_limits<int32>::max)(), 15);
	}

private:
	static bool read_range(const ConfigReader& config, const string& section,
		const string& key, int32& value, int32 minimum, int32 maximum, int32 default_value = 0)
	{
		if (!config.get_int_checked(section, key, value, default_value) ||
			value < minimum || value > maximum)
		{
			cerr << "Invalid config: " << section << "." << key
				<< " (allowed " << minimum << ".." << maximum << ")" << endl;
			return false;
		}
		return true;
	}
};
