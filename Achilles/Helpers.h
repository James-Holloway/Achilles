#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

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
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	std::vector<char> result(size);
	file.read(result.data(), size);

	return result;
}

inline void OutputDebugStringAFormatted(const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	char buffer[1024] = { 0 };
	vsprintf_s(buffer, format, arguments);
	OutputDebugStringA(buffer);
	va_end(arguments);
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


#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

// Hashers for view descriptions.
namespace std
{
	// Source: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
	template <typename T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<D3D12_SHADER_RESOURCE_VIEW_DESC>
	{
		std::size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc) const noexcept
		{
			std::size_t seed = 0;

			hash_combine(seed, srvDesc.Format);
			hash_combine(seed, srvDesc.ViewDimension);
			hash_combine(seed, srvDesc.Shader4ComponentMapping);

			switch (srvDesc.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_BUFFER:
				hash_combine(seed, srvDesc.Buffer.FirstElement);
				hash_combine(seed, srvDesc.Buffer.NumElements);
				hash_combine(seed, srvDesc.Buffer.StructureByteStride);
				hash_combine(seed, srvDesc.Buffer.Flags);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1D:
				hash_combine(seed, srvDesc.Texture1D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture1D.MipLevels);
				hash_combine(seed, srvDesc.Texture1D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
				hash_combine(seed, srvDesc.Texture1DArray.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture1DArray.MipLevels);
				hash_combine(seed, srvDesc.Texture1DArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture1DArray.ArraySize);
				hash_combine(seed, srvDesc.Texture1DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				hash_combine(seed, srvDesc.Texture2D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture2D.MipLevels);
				hash_combine(seed, srvDesc.Texture2D.PlaneSlice);
				hash_combine(seed, srvDesc.Texture2D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				hash_combine(seed, srvDesc.Texture2DArray.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture2DArray.MipLevels);
				hash_combine(seed, srvDesc.Texture2DArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture2DArray.ArraySize);
				hash_combine(seed, srvDesc.Texture2DArray.PlaneSlice);
				hash_combine(seed, srvDesc.Texture2DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMS:
				// TODO check if this works
				//                hash_combine(seed, srvDesc.Texture2DMS.UnusedField_NothingToDefine);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
				hash_combine(seed, srvDesc.Texture2DMSArray.FirstArraySlice);
				hash_combine(seed, srvDesc.Texture2DMSArray.ArraySize);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE3D:
				hash_combine(seed, srvDesc.Texture3D.MostDetailedMip);
				hash_combine(seed, srvDesc.Texture3D.MipLevels);
				hash_combine(seed, srvDesc.Texture3D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				hash_combine(seed, srvDesc.TextureCube.MostDetailedMip);
				hash_combine(seed, srvDesc.TextureCube.MipLevels);
				hash_combine(seed, srvDesc.TextureCube.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
				hash_combine(seed, srvDesc.TextureCubeArray.MostDetailedMip);
				hash_combine(seed, srvDesc.TextureCubeArray.MipLevels);
				hash_combine(seed, srvDesc.TextureCubeArray.First2DArrayFace);
				hash_combine(seed, srvDesc.TextureCubeArray.NumCubes);
				hash_combine(seed, srvDesc.TextureCubeArray.ResourceMinLODClamp);
				break;
				// TODO check if this works
			case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
				hash_combine(seed, srvDesc.RaytracingAccelerationStructure.Location);
				break;
			}

			return seed;
		}
	};

	template<>
	struct hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>
	{
		std::size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc) const noexcept
		{
			std::size_t seed = 0;

			hash_combine(seed, uavDesc.Format);
			hash_combine(seed, uavDesc.ViewDimension);

			switch (uavDesc.ViewDimension)
			{
			case D3D12_UAV_DIMENSION_BUFFER:
				hash_combine(seed, uavDesc.Buffer.FirstElement);
				hash_combine(seed, uavDesc.Buffer.NumElements);
				hash_combine(seed, uavDesc.Buffer.StructureByteStride);
				hash_combine(seed, uavDesc.Buffer.CounterOffsetInBytes);
				hash_combine(seed, uavDesc.Buffer.Flags);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1D:
				hash_combine(seed, uavDesc.Texture1D.MipSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
				hash_combine(seed, uavDesc.Texture1DArray.MipSlice);
				hash_combine(seed, uavDesc.Texture1DArray.FirstArraySlice);
				hash_combine(seed, uavDesc.Texture1DArray.ArraySize);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2D:
				hash_combine(seed, uavDesc.Texture2D.MipSlice);
				hash_combine(seed, uavDesc.Texture2D.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
				hash_combine(seed, uavDesc.Texture2DArray.MipSlice);
				hash_combine(seed, uavDesc.Texture2DArray.FirstArraySlice);
				hash_combine(seed, uavDesc.Texture2DArray.ArraySize);
				hash_combine(seed, uavDesc.Texture2DArray.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE3D:
				hash_combine(seed, uavDesc.Texture3D.MipSlice);
				hash_combine(seed, uavDesc.Texture3D.FirstWSlice);
				hash_combine(seed, uavDesc.Texture3D.WSize);
				break;
			}

			return seed;
		}
	};
}

inline bool Contains(std::wstring str, std::wstring substr)
{
	return str.find(substr) != std::wstring::npos;
}

inline std::wstring StringToWString(const std::string& str)
{
	const char* cstr = str.c_str();
	wchar_t cwstr[512] = { 0 };
	MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, cstr, -1, cwstr, 512);
	return std::wstring(cwstr);
}

inline std::string WStringToString(const std::wstring& wstr)
{
	const wchar_t* cwstr = wstr.c_str();
	char cstr[512] = { 0 };
	WideCharToMultiByte(CP_UTF8, NULL, cwstr, -1, cstr, 512, NULL, NULL);
	return std::string(cstr);
}

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }