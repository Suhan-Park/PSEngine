#pragma once
// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

/*
#ifdef _DEBUG
//디버그 버전 lib
#else
//릴리즈 버전 lib
#endif // DEBUG
*/

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX // C++에서 min, max를 사용한 경우 컴파일 오류 발생

// llb에서 dll을 참조하기 때문에, 에러가 발생할 수 있음. 디버깅에 환경 변수 세팅 필요 (프로젝트 우클릭 -> 속성 -> 구성속성 -> 디버깅 -> 환경-> dll있는 폴더 경로 지정)
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "fmod_vc.lib")
#pragma comment(lib, "assimp-vc142-mtd.lib")
#pragma comment(lib, "DirectXTK12.lib")
#pragma comment(lib, "libfbxsdk.lib")

// 'error LNK2038: mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MDd_DynamicDebug' in file.obj.' 발생시
// https://github.com/NVIDIAGameWorks/PhysX/issues/115 다음 링크 참고
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "PhysXCharacterKinematic_static_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXVehicle_static_64.lib")

// Standard Windows API.
#include <windows.h> 
#include <WindowsX.h>
#include <tchar.h> 
#include <comdef.h>

// FMOD
#include "fmod.hpp"
#include "fmod_errors.h"

// assimp
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/Bitmap.h"
#include "assimp/IOSystem.hpp"
#include "assimp/IOStream.hpp"

// Fbx sdk
#include <fbxsdk.h>

// PhsyX sdk
#include "PxPhysicsAPI.h"

// Direct Input
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif //DIRECTINPUT_VERSION

// DIrectX API.
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>
#include <D3Dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <exception>
#include <dinput.h>
#include "../d3dx12.h"
#include "directxcollision.h"

// Standard C Library.
#include <cassert>

// Standard C++ Library.
#include <iostream>
#include <cmath>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <fstream>

#include "../TinyXML/tinyxml2.h"

#include "Object.h"
#include "Util.h"

using namespace DirectX;