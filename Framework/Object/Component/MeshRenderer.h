#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Component.h"
#include "MeshFilter.h"
#include "Material.h"

struct Subset
{
	std::string MaterialName = "None";
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
};

class MeshRenderer : public Component
{
public:

	MeshRenderer();
	~MeshRenderer();

private:

	MeshRenderer(const MeshRenderer& _rhs) = delete;
	MeshRenderer& operator = (const MeshRenderer& _rhs) = delete;
	MeshRenderer(MeshRenderer&& _rhs) = delete;
	MeshRenderer& operator = (MeshRenderer&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	void AttachMaterial(Material* _material);

private:

	const D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
	const D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
	
	void CreateConstantBufferHeapAndView();

protected:

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	bool mSkinningFlag = false;

private:

	MeshFilter* mMeshFilter = nullptr;

	// 하나의 모델에 여러 Material가 사용될 경우를 위함.
	std::vector<Subset> mSubsets;
	//std::list<std::shared_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::shared_ptr<Material>> mMaterials;

	Microsoft::WRL::ComPtr<ID3DBlob> mCPUVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mCPUIndexBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mGPUVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mGPUIndexBuffer = nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexUploadBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	UINT mObjCBIndex = 0;	
};
#endif