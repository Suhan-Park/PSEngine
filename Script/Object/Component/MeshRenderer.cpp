#include "MeshRenderer.h"
#include "D3DApplication.h"
#include "Component.h"
#include "Transform.h"
#include "MeshFilter.h"
#include "Camera.h"
#include "PrimitiveGeometry.h"
#include "Util.h"

MeshRenderer::MeshRenderer()
{

}

MeshRenderer::~MeshRenderer()
{

}

void MeshRenderer::Awake()
{
	mMeshFilter = mGameObject->GetComponent<MeshFilter>();

	ThrowIfFailed(D3DCreateBlob(mMeshFilter->VertexBufferByteSize(), &mCPUVertexBuffer));
	CopyMemory(mCPUVertexBuffer->GetBufferPointer(), mMeshFilter->Vertices().data(), mMeshFilter->VertexBufferByteSize());

	ThrowIfFailed(D3DCreateBlob(mMeshFilter->IndexBufferByteSize(), &mCPUIndexBuffer));
	CopyMemory(mCPUIndexBuffer->GetBufferPointer(), mMeshFilter->Indices().data(), mMeshFilter->IndexBufferByteSize());

	D3DApplication::Instance()->Device();

	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = D3DApplication::Instance()->CommandList();

	mGPUVertexBuffer = CreateDefaultBuffer(
		device.Get(),
		commandList.Get(),
		mMeshFilter->Vertices().data(),
		mMeshFilter->VertexBufferByteSize(),
		mVertexUploadBuffer);

	mGPUIndexBuffer = CreateDefaultBuffer(
		device.Get(),
		commandList.Get(),
		mMeshFilter->Indices().data(),
		mMeshFilter->IndexBufferByteSize(),
		mIndexUploadBuffer);

	VertexBufferView();
	IndexBufferView();
	CreateConstantBufferHeapAndView();
}

void MeshRenderer::Update(const float _deltaTime)
{
	// ���������ο� ������ ����, Transform Component�� ĳ���Ѵ�.
	Transform* transform = GetComponent<Transform>();
	assert(transform != nullptr);

	XMMATRIX world = transform->WorldMatrix();
	XMMATRIX texTransform = XMMatrixIdentity(); //transform->ScaleMatrix();

	for (auto& iter : mMaterials)
	{
		std::weak_ptr<Material> wp = iter.second;
		std::shared_ptr<Material> component = wp.lock();
		component.get()->Update(_deltaTime);
	}

	// ����Ʈ �����̳� �뵵
	/*
	for (auto& iter : mMaterials)
	{
		std::weak_ptr<Material> wp = iter;
		std::shared_ptr<Material> component = wp.lock();
		component.get()->Update(_deltaTime);
	}
	*/

	// ����� ����Ǿ��� ��, ��� ���۸� �����Ѵ�. (���̴������� ���켱�̹Ƿ�, ��ġ���ش�.)
	ObjectConstants constantBufferObj;
	XMStoreFloat4x4(&constantBufferObj.World, XMMatrixTranspose(world));
	XMStoreFloat4x4(&constantBufferObj.TexTransform, XMMatrixTranspose(texTransform));
	constantBufferObj.SkinningFlag = mSkinningFlag;
	mObjectCB->CopyData(mObjCBIndex, constantBufferObj);
}

void MeshRenderer::FixedUpdate(const float _fixedDltaTime)
{
}

void MeshRenderer::Draw(const float _deltaTime)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = D3DApplication::Instance()->CommandList();

	// ���� : Root Signature�� Descriptor Table�� �ƴ�, Root Descriptor�� ����ϱ⿡, ��� �ȵǴ� �κ�
	/*
	������ ���̺��� ���ε��ϱ� ���� ���� ������ ���� ���������ο� ���ε��Ѵ�.
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(0, handle);
	commandList->SetGraphicsRootDescriptorTable(��Ʈ �Ķ���� �ε���, ������ �ڵ� = handle);
	*/

	D3D12_VERTEX_BUFFER_VIEW vbv = VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = IndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (mSubsets.size() > 0)
	{
		// Subset ������, Material�� �ٸ��� ���� �� �����Ƿ�, �׿� ���� �������Ѵ�.
		for (auto& subset : mSubsets)
		{
			std::weak_ptr<Material> wp = mMaterials[subset.MaterialName];
			std::shared_ptr<Material> component = wp.lock();
			component->Draw(_deltaTime);

			// Object Information �׸��� �ѱ��. (1�� �׸� - register0)
			auto ObjectCB = mObjectCB->Resource();;
			commandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::OBJECT_CONSTANT, ObjectCB->GetGPUVirtualAddress());
			commandList->DrawIndexedInstanced(subset.IndexCount, 1, subset.StartIndexLocation, subset.BaseVertexLocation, 0);
		}
	}
	else
	{
		for (auto& iter : mMaterials)
		{
			std::weak_ptr<Material> wp = iter.second;
			std::shared_ptr<Material> component = wp.lock();
			component.get()->Draw(_deltaTime);
		}

		auto ObjectCB = mObjectCB->Resource();;
		commandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::OBJECT_CONSTANT, ObjectCB->GetGPUVirtualAddress());
		commandList->DrawIndexedInstanced(mMeshFilter->IndexCount(), 1, 0, 0, 0);
	}
	
	// ����Ʈ �����̳� �뵵
	/*
	for (auto& iter : mMaterials)
	{
		std::weak_ptr<Material> wp = iter;
		std::shared_ptr<Material> component = wp.lock();
		component.get()->Draw(_deltaTime);
	}

	// Object Information �׸��� �ѱ��. (1�� �׸� - register0)
	auto ObjectCB = mObjectCB->Resource();;
	commandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::OBJECT_CONSTANT, ObjectCB->GetGPUVirtualAddress());
	
	commandList->DrawIndexedInstanced(mMeshFilter->IndexCount(), 1, 0, 0, 0);
	*/
}

void MeshRenderer::Destroy()
{
	mMaterials.clear();
}

void MeshRenderer::AttachMaterial(Material* _material)
{
	_material->Awake();
	//mMaterials.emplace_back(_material);
	std::shared_ptr<Material> shared(_material);
	mMaterials[_material->Name()] = shared;
}

const D3D12_VERTEX_BUFFER_VIEW MeshRenderer::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = mGPUVertexBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = mMeshFilter->VertexByteStride();
	vbv.SizeInBytes = mMeshFilter->VertexBufferByteSize();

	return vbv;
}

const D3D12_INDEX_BUFFER_VIEW MeshRenderer::IndexBufferView() const
{
	 D3D12_INDEX_BUFFER_VIEW ibv;
	 ibv.BufferLocation = mGPUIndexBuffer->GetGPUVirtualAddress();
	 ibv.Format = mMeshFilter->IndexFormat();
	 ibv.SizeInBytes = mMeshFilter->IndexBufferByteSize();

	 return ibv;
}

void MeshRenderer::CreateConstantBufferHeapAndView()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device.Get(), 1, true);

	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	
	// Root Signature�� Descriptor Table�� �ƴ�, Root Descriptor�� ����ϱ⿡, ��� �ȵǴ� �κ�
	// Descriptor�� �̿��ؼ� Heap ���� -> Heap�� ��� ���� ���� (�� ��, ��� ���� �並 �̿��ؼ� �Ӽ� ����)
	/*
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
	auto objectCB = mObjectCB->Resource();

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
	
	// 1�� ������Ʈ�� ���� ��� ���۸� �����ϹǷ�, 0���� �����ص� ����
	cbAddress += mObjCBIndex * objCBByteSize;

	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.SizeInBytes = objCBByteSize;
	cbvDesc.BufferLocation = cbAddress;

	// Heap�� ��� ���� ����
	device->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	*/

}
