#pragma once

#include <iostream>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <cassert>
#include <chrono>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <list>
#include <deque>
#include <mutex>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <intsafe.h>
#include <wrl.h>

#pragma warning( push )
#pragma warning( disable : 6001 26451 26827)
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#pragma warning( pop )

#include <DirectXTex.h>

#include <directxtk12/SimpleMath.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/VertexTypes.h>
#include <directxtk12/Keyboard.h>
#include <directxtk12/Mouse.h>

#include <pix3.h>

#include "Helpers.h"
#include "MathHelpers.h"

#include "Profiling.h"

#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment (lib, "dxcompiler.lib" )