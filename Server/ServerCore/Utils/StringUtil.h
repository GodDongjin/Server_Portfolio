#pragma once

class StringUtil
{
public:
	static wstring stringToWstring(const string& str);
	static string wstringToString(const wstring& wstr);

	static string WStringToUtf8(const wstring& wstr);
	static wstring Utf8ToWString(const string& str);
};

