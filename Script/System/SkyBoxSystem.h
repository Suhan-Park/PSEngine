#ifndef _SKYBOX_SYSTEM_H_
#define _SKYBOX_SYSTEM_H_

#include "MeshFilter.h"
#include "Texture.h"

class SkyBoxSystem final
{
private:

	SkyBoxSystem() = default;
	~SkyBoxSystem() = default;
	SkyBoxSystem(const SkyBoxSystem& _rhs) = delete;
	SkyBoxSystem& operator = (const SkyBoxSystem& _rhs) = delete;
	SkyBoxSystem(SkyBoxSystem&& _rhs) = delete;
	SkyBoxSystem& operator = (SkyBoxSystem&& _rhs) = delete;

public:

	static SkyBoxSystem* Instance();

public:

	void Initialize(std::string _name, std::wstring _fileName);
	void Release();
	void Update();
	void Draw();

private:

	const D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
	const D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

	void CreateConstantBufferHeapAndView();
	void CreateShaderResourceHeap();
	void CreateShaderResourceView();

private:

	std::unique_ptr<class Texture> mSkyMap = nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	
	MeshFilter* mMeshFilter;

	Microsoft::WRL::ComPtr<ID3DBlob> mCPUVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mCPUIndexBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mGPUVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGPUIndexBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexUploadBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
};

#endif // _SKYBOX_SYSTEM_H_
