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
	// 파이프라인에 제출을 위해, Transform Component를 캐싱한다.
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

	// 리스트 컨테이너 용도
	/*
	for (auto& iter : mMaterials)
	{
		std::weak_ptr<Material> wp = iter;
		std::shared_ptr<Material> component = wp.lock();
		component.get()->Update(_deltaTime);
	}
	*/

	// 상수가 변경되었을 때, 상수 버퍼를 갱신한다. (셰이더에서는 열우선이므로, 전치해준다.)
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

	// 수정 : Root Signature에 Descriptor Table이 아닌, Root Descriptor를 사용하기에, 사용 안되는 부분
	/*
	서술자 테이블을 바인딩하기 위해 먼저 서술자 힙을 파이프라인에 바인딩한다.
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(0, handle);
	commandList->SetGraphicsRootDescriptorTable(루트 파라미터 인덱스, 서술자 핸들 = handle);
	*/

	D3D12_VERTEX_BUFFER_VIEW vbv = VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = IndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (mSubsets.size() > 0)
	{
		// Subset 개별로, Material을 다르게 가질 수 있으므로, 그에 따라 렌더링한다.
		for (auto& subset : mSubsets)
		{
			std::weak_ptr<Material> wp = mMaterials[subset.MaterialName];
			std::shared_ptr<Material> component = wp.lock();
			component->Draw(_deltaTime);

			// Object Information 항목을 넘긴다. (1번 항목 - register0)
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
	
	// 리스트 컨테이너 용도
	/*
	for (auto& iter : mMaterials)
	{
		std::weak_ptr<Material> wp = iter;
		std::shared_ptr<Material> component = wp.lock();
		component.get()->Draw(_deltaTime);
	}

	// Object Information 항목을 넘긴다. (1번 항목 - register0)
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
	
	// Root Signature에 Descriptor Table이 아닌, Root Descriptor를 사용하기에, 사용 안되는 부분
	// Descriptor를 이용해서 Heap 생성 -> Heap에 상수 버퍼 생성 (이 때, 상수 버퍼 뷰를 이용해서 속성 서술)
	/*
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
	auto objectCB = mObjectCB->Resource();

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
	
	// 1개 오브젝트에 대한 상수 버퍼를 생성하므로, 0으로 지정해도 무관
	cbAddress += mObjCBIndex * objCBByteSize;

	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.SizeInBytes = objCBByteSize;
	cbvDesc.BufferLocation = cbAddress;

	// Heap에 상수 버퍼 생성
	device->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	*/

}
