#pragma once
#include "../Utils/Types.h"

class ConfigReader
{
public:
	ConfigReader(wstring path);

public:
	bool load();

	string get_string(const string& section, const string& key, const string& default_value = "")const;
	wstring get_wstring(const string& section, const string& key, const wstring& default_value = L"")const;
	int32 get_int(const string& section, const string& key, int32 default_value = 0)const;

private:
	string trim(const string& value);

private:
	wstring _path;
	map<string, map<string, string>> _sections;
};
