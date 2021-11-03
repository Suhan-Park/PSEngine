#ifndef _UTIL_H_
#define _UTIL_H_

#include "Math.h"

enum class ROOT_SIGNATURE_ID : UINT
{
	TEXTURE = 0,
	SHADOW_MAP,
	SSAO_MAP,
	SKY_BOX,
	OBJECT_CONSTANT,
	PASS_CONSTANT,
	LIGHT,
	MATERIAL,
	SKINNED
};

struct Bone
{
	std::string Name;
	INT Index;
	INT ParentIndex;
	XMFLOAT4X4 LocalTransform;
	XMFLOAT4X4 LocalOffsetTransform;
	XMFLOAT4X4 RootTransform; // 루트 기준으로 각 Bone을 공간변환 (루트 변환)
	XMFLOAT4X4 OffsetTransform;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException
{
public:

	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr),
		FunctionName(functionName),
		Filename(filename),
		LineNumber(lineNumber)
	{ }

	std::wstring ToString()const
	{
		// 에러 코드의 문자열 설명을 가져온다.
		_com_error err(ErrorCode);
		std::wstring msg = err.ErrorMessage();

		return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
	}


	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;

};

static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& _filename,
	const D3D_SHADER_MACRO* _defines,
	const std::string& _entrypoint,
	const std::string& _target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(_filename.c_str(), _defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		_entrypoint.c_str(), _target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

// 실제 정점 버퍼 자원을 생성하는 것과 더불어, 어플리케이션은 D3D12_HEAP_TYPE_UPLOAD 형식의 힙에 임시 업로드용 버퍼를 생성해야한다.
// CPU 메모리에서 GPU 메모리로 자료를 복사하려면 업로드 힙에 자원을 맡겨야한다.
// 1. 업로드 버퍼를 생성하고
// 2. 시스템 메모리에 있는 정점 자료를 업로드 버퍼에 복사하고
// 3. 그런 다음에 업로드 버퍼의 정점 자료를 실제 정점 버퍼로 복사한다. (정점 버퍼는 기본힙(D3D12_HEAP_TYPE_DEFAULT)에 넣어진다.)
//
// CPU 메모리                                 GPU 메모리
// [정점 자료] ------[임시 업로드 버퍼]-----> [정점 버퍼]

// 기본 버퍼의 자료를 초기화하려면 항상 임시 업로드 버퍼가 필요하다.
static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

    // 실제 기본 버퍼 자원을 생성한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
       
    // CPU 메모리의 자료를 기본 버퍼에 복사하려면, 임시 업로드 힙을 만들어야한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // 기본 버퍼에 복사할 자료를 서술한다.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // 기본 버퍼의 자원으로의 자료 복사를 요청한다.
    // 개략적으로는, 보조 함수 UpdateSubresources는 CPU 메모리를 임시 업로드 힙에 복사하고
    // ID3D12CommandList::CopySubresourceRegion을 이용해서 임시 업로드 힙의 자료를  mBuffer에 복사한다.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // 주의 : 위의 함수 호출 이후에도 uploadBuffer를 계속 유지해야 한다.
    // 실제로 복사를 수행하는 명령 목록이 아직 실행되지 않았기 때문이다.
    // 복사가 완료되었음이 확실해진 후에 호출자가 uploadBuffer를 해제하면 된다.

    return defaultBuffer;
}

// 상수 버퍼는 정점, 인덱스 버퍼와는 달리 보통 CPU가 매 프레임당 갱신한다.
// 카메라가 매 프레임 이동한다면, 프레임마다 상수 버퍼를 새 View 행렬로 업데이트 해야함.
// 따라서 상수 버퍼는 업로드 힙에 만들어져야 한다.
// 정점 버퍼와 인덱스 버퍼는 기본 힙에 만들어 졌음.
// 그리고 상수 버퍼는 크기가 256 바이트의 배수이어야 한다.
// 씬의 물체가 n개이면, 상수 버퍼도 n개가 필요하다.
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = Math::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = Math::Identity4x4();
	UINT SkinningFlag = 0;
};

struct SkinnedConstants
{
	DirectX::XMFLOAT4X4 BoneTransforms[96];
};

struct SSAOConstants
{
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4X4 ProjTex;
	DirectX::XMFLOAT4   OffsetVectors[14];

	// SsaoBlur.hlsl에서 사용한다.
	DirectX::XMFLOAT4 BlurWeights[3];

	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	FLOAT OcclusionRadius = 0.5f;
	FLOAT OcclusionFadeStart = 0.2f;
	FLOAT OcclusionFadeEnd = 2.0f;
	FLOAT SurfaceEpsilon = 0.05f;
};

struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.08f, 0.08f, 0.08f };
	float Shininess = 1.0f;
	DirectX::XMFLOAT2 Tiling = { 1.0f, 1.0f };
	DirectX::XMFLOAT2 Offset = { 0.0f, 0.0f };
	UINT DiffuseMapFlag = 0;
	UINT NormalMapFlag = 0;
	XMFLOAT2 Pad1 = XMFLOAT2(1.0f,1.0f);
	DirectX::XMFLOAT4X4 MatTransform = Math::Identity4x4();
};

struct LightData
{
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FLOAT Pad = 0.0f;
	XMFLOAT3 Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FLOAT Pad1 = 0.0f;
	XMFLOAT4 Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	FLOAT Range = 1.0f;
	FLOAT Intensity = 1.0f;
	FLOAT SpotAngle = 30.0f;
	FLOAT Pad2 = 0.0f;
};

struct PassConstants
{
	XMFLOAT4X4 View = Math::Identity4x4();
	XMFLOAT4X4 InvView = Math::Identity4x4();
	XMFLOAT4X4 Proj = Math::Identity4x4();
	XMFLOAT4X4 InvProj = Math::Identity4x4();
	XMFLOAT4X4 ViewProj = Math::Identity4x4();
	XMFLOAT4X4 InvViewProj = Math::Identity4x4();
	XMFLOAT4X4 ViewProjTex = Math::Identity4x4();
	XMFLOAT4X4 Shadow = Math::Identity4x4();
	XMFLOAT4 AmbientLight = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	FLOAT Pad = 0.0f;
	XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	FLOAT NearZ = 0.0f;
	FLOAT FarZ = 0.0f;
	FLOAT TotalTime = 0.0f;
	FLOAT DeltaTime = 0.0f;
};

struct PassLight
{
	int DirectionalLightCount = 0;
	int PointLightCount = 0;
	int SpotLightCount = 0;
	int Pad = 0;
	LightData Light[15];
};

static UINT CalcConstantBufferByteSize(UINT byteSize)
{
    // 상수 버퍼의 크기는 반드시 최소 하드웨어 할당 크기(흔히 256바이트)
    // 배수이어야 한다. 이 함수는 주어진 크기에 가장 가까운 256의 배수를 계산해서 돌려준다.
    // 이를 위해 이 함수는 크기에 255를 더하고 비트 마스크를 이용해서 하위 2바이트,
    // 즉 256보다 작은 모든 비트를 0으로 만든다. 
    //예 : byteSize = 300 이라고 할 때,
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x0200
    // 512

    return (byteSize + 255) & ~255;
}

static std::string GetFilePathUsingAbsolutePath(std::string _path)
{
	// User\Desktop\Project/Resources/Sample.png - 이렇게 되어있는 경우 , Resources/Sample.png 를 추출하기 위함

	int split = _path.rfind("\\") + 1;
	std::string filePath = _path.substr(split, _path.length() - split);

	return filePath;
}
// 경로에 해당하는 파일이 저장된 폴더 명을 반환한다.
static std::string GetDirectoryName(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	// 디렉토리가 A\\B\\C\\test.txt 일 때, A\\B\\C와 test.txt로 구분한다.
	int split = path.rfind("/");
	std::string filePath = path.substr(0, split);
	//std::string filePath = path.substr(split, static_cast<int>(path.length()));

	// 디렉토리가 A\\B\\C\\test.txt 일 때, C 폴더 이름을 추출한다.
	split = filePath.rfind("/") + 1;
	std::string folderName = filePath.substr(split, static_cast<int>(filePath.length()));

	return folderName;
}

// 경로에 해당하는 파일이 저장된 폴더 명을 반환한다.
static std::string GetDirectoryName(std::string _path)
{
	// 디렉토리가 A\\B\\C\\test.txt 일 때, A\\B\\C와 test.txt로 구분한다.
	int split = _path.rfind("/");
	std::string filePath = _path.substr(0, split);
	//std::string filePath = path.substr(split, static_cast<int>(path.length()));

	// 디렉토리가 A\\B\\C\\test.txt 일 때, C 폴더 이름을 추출한다.
	split = filePath.rfind("/") + 1;
	std::string folderName = filePath.substr(split, static_cast<int>(filePath.length()));

	return folderName;
}

// 경로에 해당하는 파일이 저장된 디렉토리 경로를 반환한다.
static std::string GetDirectoryPath(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	// 디렉토리가 A\\B\\C\\test.txt 일 때, A\\B\\C와 test.txt로 구분한다.
	int split = path.rfind("/");
	std::string folderPath = path.substr(0, split);
	return folderPath;
}

// 경로에 해당하는 파일이 저장된 디렉토리 경로를 반환한다.
static std::string GetDirectoryPath(std::string _path)
{
	// 디렉토리가 A\\B\\C\\test.txt 일 때, A\\B\\C와 test.txt로 구분한다.
	int split = _path.rfind("/");
	std::string folderPath = _path.substr(0, split);
	return folderPath;
}

// 경로에 해당하는 파일 명을 반환한다.
static std::string GetFileName(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	int split = path.rfind("/") + 1;
	std::string name = path.substr(split, _path.length() - split);

	return name;
}

// 경로에 해당하는 파일 명을 반환한다.
static std::string GetFileName(std::string _path)
{
	int split = _path.rfind("/") + 1;
	std::string name = _path.substr(split, _path.length() - split);

	return name;
}

// 경로에 해당하는 파일 명을 반환한다.
static std::string GetFileNameWithoutExtension(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	int split = path.rfind("/") + 1;
	std::string name = path.substr(split, _path.length() - split);

	int ext = name.rfind(".");
	name = name.substr(0, ext);

	return name;
}

// 경로에 해당하는 파일 명을 반환한다.
static std::string GetFileNameWithoutExtension(std::string _path)
{
	int split = _path.rfind("/") + 1;
	std::string name = _path.substr(split, _path.length() - split);

	int ext = name.rfind(".");
	name = name.substr(0, ext);

	return name;
}

// aiTexture 파일 이름을 가지고 경로를 구성한다.
static std::string GetTextureFilePath(std::string _fileName, std::string _folderPath)
{
	const std::string finder = "\\";
	const std::string replacer = "/";

	if (std::string::npos != _fileName.find(finder))
	{
		_fileName.replace(_fileName.find(finder), finder.length(), replacer);
	}

	std::string filePath = _folderPath + "/" + GetFileNameWithoutExtension(_fileName);

	return filePath;
}

static std::string Convert4x4MatrixtoString(XMFLOAT4X4 _matrix)
{
	std::string res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			std::string put = std::to_string(_matrix.m[i][j]) + ",";
			res += (put.c_str());
		}
	}
	return res;
}

template<typename T>
class UploadBuffer
{
public:

    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);

        // 상수 버퍼 원소의 크기는 반드시 256바이트의 배수이어야 한다.
        // 이는 하드웨어가 m*256 바이트 오프셋에서 시작하는 n*256 바이트 길이의 상수 자료만 볼 수 있기 때문이다.
        if (isConstantBuffer)
        {
            mElementByteSize = CalcConstantBufferByteSize(sizeof(T));
        }
        
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // 자원을 다 사용하기 전에는 Map을 해제할 필요가 없다.
        // 그러나 자원을 GPU가 사용하는 중에는 CPU에서 자원을 갱신하지 않아야 한다.
        // (따라서 반드시 동기화가 적용되어야 한다.)
    }
    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
        {
            mUploadBuffer->Unmap(0, nullptr);
        }
    }

private:

    UploadBuffer(UploadBuffer& _rhs) = delete;
    UploadBuffer& operator = (UploadBuffer& _rhs) = delete;
    UploadBuffer(UploadBuffer&& _rhs) = delete;
    UploadBuffer& operator = (UploadBuffer&& _rhs) = delete;

public:

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;

};

#endif // !_UTIL_H_
