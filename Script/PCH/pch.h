#pragma once
// pch.h: �̸� �����ϵ� ��� �����Դϴ�.
// �Ʒ� ������ ������ �� ���� �����ϵǾ�����, ���� ���忡 ���� ���� ������ ����մϴ�.
// �ڵ� ������ �� ���� �ڵ� �˻� ����� �����Ͽ� IntelliSense ���ɿ��� ������ ��Ĩ�ϴ�.
// �׷��� ���⿡ ������ ������ ���� �� ������Ʈ�Ǵ� ��� ��� �ٽ� �����ϵ˴ϴ�.
// ���⿡ ���� ������Ʈ�� ������ �߰����� ������. �׷��� ������ ���ϵ˴ϴ�.

/*
#ifdef _DEBUG
//����� ���� lib
#else
//������ ���� lib
#endif // DEBUG
*/

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX // C++���� min, max�� ����� ��� ������ ���� �߻�

// llb���� dll�� �����ϱ� ������, ������ �߻��� �� ����. ����뿡 ȯ�� ���� ���� �ʿ� (������Ʈ ��Ŭ�� -> �Ӽ� -> �����Ӽ� -> ����� -> ȯ��-> dll�ִ� ���� ��� ����)
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "fmod_vc.lib")
#pragma comment(lib, "assimp-vc142-mtd.lib")
#pragma comment(lib, "DirectXTK12.lib")
#pragma comment(lib, "libfbxsdk.lib")

// 'error LNK2038: mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MDd_DynamicDebug' in file.obj.' �߻���
// https://github.com/NVIDIAGameWorks/PhysX/issues/115 ���� ��ũ ����
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