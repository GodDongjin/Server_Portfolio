#include "ConfigReader.h"
#include "../Utils/StringUtil.h"

ConfigReader::ConfigReader(wstring path) : _path(path)
{
}

bool ConfigReader::load()
{
	ifstream file(_path);

	if (!file.is_open()) {
		return false;
	}

	string current_section;
	string line;

	while (getline(file, line))
	{
		line = trim(line);

		if (line.empty()) {
			continue;
		}

		if (line[0] == '#' || line[0] == ';') {
			continue;
		}

		if (line.front() == '[' && line.back() == ']')
		{
			current_section = trim(line.substr(1, line.size() - 2));
			continue;
		}

		const size_t equal_pos = line.find('=');
		if (equal_pos == string::npos) {
			continue;
		}

		string key = trim(line.substr(0, equal_pos));
		string value = trim(line.substr(equal_pos + 1));

		if (current_section.empty() || key.empty()) {
			continue;
		}

		_sections[current_section][key] = value;
	}

	return true;
}

string ConfigReader::get_string(const string& section, const string& key, const string& default_value) const
{
	auto section_iter = _sections.find(section);
	if (section_iter == _sections.end()) {
		return default_value;
	}

	auto key_iter = section_iter->second.find(key);
	if (key_iter == section_iter->second.end()) {
		return default_value;
	}

	return key_iter->second;
}

wstring ConfigReader::get_wstring(const string& section, const string& key, const wstring& default_value) const
{
	const string value = get_string(section, key, "");

	if (value.empty()) {
		return default_value;
	}

	return Utf8ToWString(value);
}

int32 ConfigReader::get_int(const string& section, const string& key, int32 default_value) const
{
	const string value = get_string(section, key, "");

	if (value.empty()) {
		return default_value;
	}

	try
	{
		return stoi(value);
	}
	catch (...)
	{
		return default_value;
	}
}

string ConfigReader::trim(const string& value)
{
	const char* whitespace = " \t\r\n";

	const size_t start = value.find_first_not_of(whitespace);
	if (start == string::npos) {
		return "";
	}

	const size_t end = value.find_last_not_of(whitespace);

	return value.substr(start, end - start + 1);
}
