#include "SkyBoxSystem.h"
#include "D3DApplication.h"

#include "Texture.h"

#include "PrimitiveGeometry.h"

SkyBoxSystem * SkyBoxSystem::Instance()
{
	static SkyBoxSystem instance;
	return &instance;
}

void SkyBoxSystem::Initialize(std::string _name, std::wstring _fileName)
{
	mMeshFilter = PrimitiveGeometry::Sphere();
	mMeshFilter->Awake();

	mSkyMap = std::make_unique<Texture>(_name, _fileName);

	ThrowIfFailed(D3DCreateBlob(mMeshFilter->VertexBufferByteSize(), &mCPUVertexBuffer));
	CopyMemory(mCPUVertexBuffer->GetBufferPointer(), mMeshFilter->Vertices().data(), mMeshFilter->VertexBufferByteSize());

	ThrowIfFailed(D3DCreateBlob(mMeshFilter->IndexBufferByteSize(), &mCPUIndexBuffer));
	CopyMemory(mCPUIndexBuffer->GetBufferPointer(), mMeshFilter->Indices().data(), mMeshFilter->IndexBufferByteSize());

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

	CreateShaderResourceHeap();
	CreateShaderResourceView();
}

void SkyBoxSystem::Update()
{
	XMMATRIX scaleMatrix = XMMatrixScaling(5000.0f, 5000.0f, 5000.0f);
	XMMATRIX texTransform = XMMatrixIdentity(); //transform->ScaleMatrix();

	// 상수가 변경되었을 때, 상수 버퍼를 갱신한다. (셰이더에서는 열우선이므로, 전치해준다.)
	ObjectConstants constantBufferObj;
	XMStoreFloat4x4(&constantBufferObj.World, XMMatrixTranspose(scaleMatrix));
	XMStoreFloat4x4(&constantBufferObj.TexTransform, XMMatrixTranspose(texTransform));
	mObjectCB->CopyData(0, constantBufferObj);
}

void SkyBoxSystem::Draw()
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = D3DApplication::Instance()->CommandList();

	D3D12_VERTEX_BUFFER_VIEW vbv = VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW ibv = IndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	D3DApplication::Instance()->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	D3DApplication::Instance()->CommandList()->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_ID::SKY_BOX, tex);

	// Object Information 항목을 넘긴다. (1번 항목 - register0)
	auto ObjectCB = mObjectCB->Resource();;
	commandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::OBJECT_CONSTANT, ObjectCB->GetGPUVirtualAddress());
	commandList->DrawIndexedInstanced(mMeshFilter->IndexCount(), 1, 0, 0, 0);
}

void SkyBoxSystem::Release()
{
}

void SkyBoxSystem::CreateShaderResourceView()
{
	mSkyMap->Initialize();

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(0, D3DApplication::Instance()->GetCbvSrvDescriptorSize());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mSkyMap->Get()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = mSkyMap->Get()->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	D3DApplication::Instance()->Device()->CreateShaderResourceView(mSkyMap->Get(), &srvDesc, hDescriptor);
}

const D3D12_VERTEX_BUFFER_VIEW SkyBoxSystem::VertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = mGPUVertexBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = mMeshFilter->VertexByteStride();
	vbv.SizeInBytes = mMeshFilter->VertexBufferByteSize();

	return vbv;
}

const D3D12_INDEX_BUFFER_VIEW SkyBoxSystem::IndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = mGPUIndexBuffer->GetGPUVirtualAddress();
	ibv.Format = mMeshFilter->IndexFormat();
	ibv.SizeInBytes = mMeshFilter->IndexBufferByteSize();

	return ibv;
}

void SkyBoxSystem::CreateConstantBufferHeapAndView()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device.Get(), 1, true);

	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
}

void SkyBoxSystem::CreateShaderResourceHeap()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
}