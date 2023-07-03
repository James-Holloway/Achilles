#pragma once

#include <iostream>
#include <algorithm>
#include <utility>
#include <cassert>
#include <chrono>
#include <vector>
#include <map>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#pragma warning( push )
#pragma warning( disable : 6001 26451 26827)
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;
#include <directx/d3dx12.h>
#pragma warning( pop )

#include "Helpers.h"

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )