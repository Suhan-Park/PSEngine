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
	XMFLOAT4X4 RootTransform; // ��Ʈ �������� �� Bone�� ������ȯ (��Ʈ ��ȯ)
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
		// ���� �ڵ��� ���ڿ� ������ �����´�.
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

// ���� ���� ���� �ڿ��� �����ϴ� �Ͱ� ���Ҿ�, ���ø����̼��� D3D12_HEAP_TYPE_UPLOAD ������ ���� �ӽ� ���ε�� ���۸� �����ؾ��Ѵ�.
// CPU �޸𸮿��� GPU �޸𸮷� �ڷḦ �����Ϸ��� ���ε� ���� �ڿ��� �ðܾ��Ѵ�.
// 1. ���ε� ���۸� �����ϰ�
// 2. �ý��� �޸𸮿� �ִ� ���� �ڷḦ ���ε� ���ۿ� �����ϰ�
// 3. �׷� ������ ���ε� ������ ���� �ڷḦ ���� ���� ���۷� �����Ѵ�. (���� ���۴� �⺻��(D3D12_HEAP_TYPE_DEFAULT)�� �־�����.)
//
// CPU �޸�                                 GPU �޸�
// [���� �ڷ�] ------[�ӽ� ���ε� ����]-----> [���� ����]

// �⺻ ������ �ڷḦ �ʱ�ȭ�Ϸ��� �׻� �ӽ� ���ε� ���۰� �ʿ��ϴ�.
static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

    // ���� �⺻ ���� �ڿ��� �����Ѵ�.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
       
    // CPU �޸��� �ڷḦ �⺻ ���ۿ� �����Ϸ���, �ӽ� ���ε� ���� �������Ѵ�.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // �⺻ ���ۿ� ������ �ڷḦ �����Ѵ�.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // �⺻ ������ �ڿ������� �ڷ� ���縦 ��û�Ѵ�.
    // ���������δ�, ���� �Լ� UpdateSubresources�� CPU �޸𸮸� �ӽ� ���ε� ���� �����ϰ�
    // ID3D12CommandList::CopySubresourceRegion�� �̿��ؼ� �ӽ� ���ε� ���� �ڷḦ  mBuffer�� �����Ѵ�.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // ���� : ���� �Լ� ȣ�� ���Ŀ��� uploadBuffer�� ��� �����ؾ� �Ѵ�.
    // ������ ���縦 �����ϴ� ��� ����� ���� ������� �ʾұ� �����̴�.
    // ���簡 �Ϸ�Ǿ����� Ȯ������ �Ŀ� ȣ���ڰ� uploadBuffer�� �����ϸ� �ȴ�.

    return defaultBuffer;
}

// ��� ���۴� ����, �ε��� ���ۿʹ� �޸� ���� CPU�� �� �����Ӵ� �����Ѵ�.
// ī�޶� �� ������ �̵��Ѵٸ�, �����Ӹ��� ��� ���۸� �� View ��ķ� ������Ʈ �ؾ���.
// ���� ��� ���۴� ���ε� ���� ��������� �Ѵ�.
// ���� ���ۿ� �ε��� ���۴� �⺻ ���� ����� ����.
// �׸��� ��� ���۴� ũ�Ⱑ 256 ����Ʈ�� ����̾�� �Ѵ�.
// ���� ��ü�� n���̸�, ��� ���۵� n���� �ʿ��ϴ�.
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

	// SsaoBlur.hlsl���� ����Ѵ�.
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
    // ��� ������ ũ��� �ݵ�� �ּ� �ϵ���� �Ҵ� ũ��(���� 256����Ʈ)
    // ����̾�� �Ѵ�. �� �Լ��� �־��� ũ�⿡ ���� ����� 256�� ����� ����ؼ� �����ش�.
    // �̸� ���� �� �Լ��� ũ�⿡ 255�� ���ϰ� ��Ʈ ����ũ�� �̿��ؼ� ���� 2����Ʈ,
    // �� 256���� ���� ��� ��Ʈ�� 0���� �����. 
    //�� : byteSize = 300 �̶�� �� ��,
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x0200
    // 512

    return (byteSize + 255) & ~255;
}

static std::string GetFilePathUsingAbsolutePath(std::string _path)
{
	// User\Desktop\Project/Resources/Sample.png - �̷��� �Ǿ��ִ� ��� , Resources/Sample.png �� �����ϱ� ����

	int split = _path.rfind("\\") + 1;
	std::string filePath = _path.substr(split, _path.length() - split);

	return filePath;
}
// ��ο� �ش��ϴ� ������ ����� ���� ���� ��ȯ�Ѵ�.
static std::string GetDirectoryName(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	// ���丮�� A\\B\\C\\test.txt �� ��, A\\B\\C�� test.txt�� �����Ѵ�.
	int split = path.rfind("/");
	std::string filePath = path.substr(0, split);
	//std::string filePath = path.substr(split, static_cast<int>(path.length()));

	// ���丮�� A\\B\\C\\test.txt �� ��, C ���� �̸��� �����Ѵ�.
	split = filePath.rfind("/") + 1;
	std::string folderName = filePath.substr(split, static_cast<int>(filePath.length()));

	return folderName;
}

// ��ο� �ش��ϴ� ������ ����� ���� ���� ��ȯ�Ѵ�.
static std::string GetDirectoryName(std::string _path)
{
	// ���丮�� A\\B\\C\\test.txt �� ��, A\\B\\C�� test.txt�� �����Ѵ�.
	int split = _path.rfind("/");
	std::string filePath = _path.substr(0, split);
	//std::string filePath = path.substr(split, static_cast<int>(path.length()));

	// ���丮�� A\\B\\C\\test.txt �� ��, C ���� �̸��� �����Ѵ�.
	split = filePath.rfind("/") + 1;
	std::string folderName = filePath.substr(split, static_cast<int>(filePath.length()));

	return folderName;
}

// ��ο� �ش��ϴ� ������ ����� ���丮 ��θ� ��ȯ�Ѵ�.
static std::string GetDirectoryPath(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	// ���丮�� A\\B\\C\\test.txt �� ��, A\\B\\C�� test.txt�� �����Ѵ�.
	int split = path.rfind("/");
	std::string folderPath = path.substr(0, split);
	return folderPath;
}

// ��ο� �ش��ϴ� ������ ����� ���丮 ��θ� ��ȯ�Ѵ�.
static std::string GetDirectoryPath(std::string _path)
{
	// ���丮�� A\\B\\C\\test.txt �� ��, A\\B\\C�� test.txt�� �����Ѵ�.
	int split = _path.rfind("/");
	std::string folderPath = _path.substr(0, split);
	return folderPath;
}

// ��ο� �ش��ϴ� ���� ���� ��ȯ�Ѵ�.
static std::string GetFileName(std::wstring _path)
{
	std::string path;
	path.assign(_path.begin(), _path.end());

	int split = path.rfind("/") + 1;
	std::string name = path.substr(split, _path.length() - split);

	return name;
}

// ��ο� �ش��ϴ� ���� ���� ��ȯ�Ѵ�.
static std::string GetFileName(std::string _path)
{
	int split = _path.rfind("/") + 1;
	std::string name = _path.substr(split, _path.length() - split);

	return name;
}

// ��ο� �ش��ϴ� ���� ���� ��ȯ�Ѵ�.
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

// ��ο� �ش��ϴ� ���� ���� ��ȯ�Ѵ�.
static std::string GetFileNameWithoutExtension(std::string _path)
{
	int split = _path.rfind("/") + 1;
	std::string name = _path.substr(split, _path.length() - split);

	int ext = name.rfind(".");
	name = name.substr(0, ext);

	return name;
}

// aiTexture ���� �̸��� ������ ��θ� �����Ѵ�.
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

        // ��� ���� ������ ũ��� �ݵ�� 256����Ʈ�� ����̾�� �Ѵ�.
        // �̴� �ϵ��� m*256 ����Ʈ �����¿��� �����ϴ� n*256 ����Ʈ ������ ��� �ڷḸ �� �� �ֱ� �����̴�.
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

        // �ڿ��� �� ����ϱ� ������ Map�� ������ �ʿ䰡 ����.
        // �׷��� �ڿ��� GPU�� ����ϴ� �߿��� CPU���� �ڿ��� �������� �ʾƾ� �Ѵ�.
        // (���� �ݵ�� ����ȭ�� ����Ǿ�� �Ѵ�.)
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
