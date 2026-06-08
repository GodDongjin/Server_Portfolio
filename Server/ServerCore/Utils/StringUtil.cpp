#include "pch.h"
#include "StringUtil.h"

std::wstring StringUtil::stringToWstring(const std::string& str) {
	// MultiByteToWideChar를 사용하여 UTF-8 문자열을 UTF-16 문자열로 변환
	int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

	// 변환된 문자열을 저장할 wstring 생성
	std::wstring wideString(wideCharSize, 0);

	// 변환된 내용을 wstring에 저장
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideString[0], wideCharSize);

	return wideString;
}

std::string StringUtil::wstringToString(const std::wstring& wstr) {
	// WideCharToMultiByte를 사용하여 UTF-16 문자열을 UTF-8 문자열로 변환
	int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);

	// 변환된 문자열을 저장할 string 생성
	std::string utf8String(utf8Size - 1, 0);  // null terminator 제외

	// 변환된 내용을 string에 저장
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Size, nullptr, nullptr);

	return utf8String;
}

std::string StringUtil::WStringToUtf8(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";

	int len = ::WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), static_cast<int>(wstr.size()),
		nullptr, 0,
		nullptr, nullptr
	);

	std::string result(len, 0);

	::WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), static_cast<int>(wstr.size()),
		result.data(), len,
		nullptr, nullptr
	);

	return result;
}

std::wstring StringUtil::Utf8ToWString(const std::string& str)
{
	if (str.empty())
		return L"";

	int len = ::MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), static_cast<int>(str.size()),
		nullptr, 0
	);

	std::wstring result(len, 0);

	::MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), static_cast<int>(str.size()),
		result.data(), len
	);

	return result;
}
