#pragma once

#include <iostream>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <cassert>
#include <chrono>
#include <vector>
#include <map>
#include <queue>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <intsafe.h>
#include <wrl.h>

#pragma warning( push )
#pragma warning( disable : 6001 26451 26827)
#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <directx/d3dx12.h>
#pragma warning( pop )

#include <directxtk12/SimpleMath.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>

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

using namespace DirectX;
using namespace Microsoft::WRL;