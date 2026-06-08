#pragma once
#include "../Utils/CorePch.h"

class ConfigReader
{
public:
	ConfigReader(wstring path);

public:
	bool load();

    string get_string(const string& section, const string& key, const string& default_value = "");
    wstring get_wstring(const string& section, const string& key, const wstring& default_value = L"");
    int32 get_int( const string& section, const string& key, int32 default_value = 0);

private:
    string trim(const string& value);

private:
	wstring _path;
	map<string, map<string, string>> _sections;
};

