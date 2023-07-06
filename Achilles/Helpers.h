#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
		throw std::exception();
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

inline std::string GetExePath()
{
	char result[MAX_PATH];
	return std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
}
inline std::wstring GetExePathW()
{
	wchar_t result[MAX_PATH];
	return std::wstring(result, GetModuleFileNameW(NULL, result, MAX_PATH));
}

inline std::string GetDirectoryFromPath(std::string path)
{
	const size_t lastSlash = path.rfind('\\');
	if (std::string::npos != lastSlash)
		return path.substr(0, lastSlash);
	return path;
}
inline std::wstring GetDirectoryFromPath(std::wstring path)
{
	const size_t lastSlash = path.rfind('\\');
	if (std::wstring::npos != lastSlash)
		return path.substr(0, lastSlash);
	return path;
}

inline std::string GetContentDirectory()
{
	return GetDirectoryFromPath(GetExePath()) + "/content/";
}
inline std::wstring GetContentDirectoryW()
{
	return GetDirectoryFromPath(GetExePathW()) + L"/content/";
}

inline bool FileExists(std::string path)
{
	std::filesystem::path file{path};
	return std::filesystem::exists(file);
}
inline bool FileExists(std::wstring path)
{
	std::filesystem::path file{path};
	return std::filesystem::exists(file);
}

inline std::vector<char> ReadFile(std::string path)
{
	uintmax_t size = std::filesystem::file_size(path);

	if (size == 0) {
		return std::vector<char>{};
	}
	std::ifstream file(path, std::ios::binary|std::ios::ate);

	std::vector<char> result(size);
	file.read(result.data(), size);

	return result;
}

inline void OutputDebugStringWFormatted(const wchar_t* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	wchar_t buffer[1024] = { 0 };
	vswprintf_s(buffer, format, arguments);
	OutputDebugStringW(buffer);
	va_end(arguments);
}