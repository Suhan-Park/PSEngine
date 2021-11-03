#include "Material.h"
#include "D3DApplication.h"

// Srv Heap에 리소스를 한곳에 다 저장하고
// 루트 서명(Draw할 때)은 여러 슬롯에 나누어 바인딩 할 수 있다. (예 : 1번 슬롯, 2번슬롯 , ...)
// 대신 바인딩 할 때, 위치를 알아야함.
// 예를들어, DiffuseMap, NormalMap, SpecularMap, CubeMap을 Srv Heap 한 곳에 다 저장했을 때,
// 1번 Descriptor Table에는 D, N, P를 바인딩하고, 2번 Descriptor Table에는 Cube Map을 바인딩 하려고 했을 때,
// Descriptor.Offset(CubeMapOffset, SrvDescriptorSize)만큼 밀어서 핸들을 넘겨주어야 한다.
/*
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetGraphicsRootDescriptorTable(0, tex); // Diffuse, Normal, Specular Map
	Descriptor.Offset(CubeMapOffset, SrvDescriptorSize); // Offset만큼 이동
	CommandList->SetGraphicsRootDescriptorTable(1, tex); // Cube Map
*/

Material::Material(std::string _name)
{
	SetName(_name);
}

void Material::Awake()
{
	mMaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(D3DApplication::Instance()->Device().Get(), 1, true);
}

void Material::Update(const float _deltaTime)
{
	UpdateMaterialBuffer();
}

void Material::Draw(const float _deltaTime)
{

	auto MaterialCB = mMaterialBuffer->Resource();
	D3DApplication::Instance()->CommandList()->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::MATERIAL, MaterialCB->GetGPUVirtualAddress());
	
	if (nullptr == mDiffuseMap)
	{
		return;
	}

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	D3DApplication::Instance()->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	D3DApplication::Instance()->CommandList()->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_ID::TEXTURE, tex);
}

void Material::Destroy()
{
	mDiffuseMap = nullptr;
	mNormalMap = nullptr;
	mSpecularMap = nullptr;
}

void Material::CreateShaderResourceHeapAndView()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(D3DApplication::Instance()->Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
}

void Material::UpdateMaterialBuffer()
{
	MaterialData materialBufferObj;
	materialBufferObj.DiffuseAlbedo = mAlbedo;
	materialBufferObj.FresnelR0 = mFresnelR0;
	materialBufferObj.Shininess = std::min(mShineness, 1.0f);
	materialBufferObj.Tiling = mTiling;
	materialBufferObj.Offset = mOffset;
	materialBufferObj.DiffuseMapFlag = mDiffuseMapFlag;
	materialBufferObj.NormalMapFlag = mNormalMapFlag;

	XMMATRIX materialTransform = XMLoadFloat4x4(&mMaterialData.MatTransform);

	// Material이 변경되었을 때, Material 버퍼를 갱신한다. (셰이더에서는 열우선이므로, 전치해준다.)
	XMStoreFloat4x4(&materialBufferObj.MatTransform, XMMatrixTranspose(materialTransform));

	mMaterialBuffer->CopyData(0, materialBufferObj);
}

void Material::AttachDiffuseMap(Texture* _texture)
{
	if (nullptr == _texture)
	{
		return;
	}

	if (nullptr == mSrvDescriptorHeap)
	{
		CreateShaderResourceHeapAndView();
	}

	mDiffuseMap = _texture;
	mDiffuseMapFlag = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(0, D3DApplication::Instance()->GetCbvSrvDescriptorSize());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mDiffuseMap->Get()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mDiffuseMap->Get()->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	D3DApplication::Instance()->Device()->CreateShaderResourceView(mDiffuseMap->Get(), &srvDesc, hDescriptor);
}

void Material::AttachNormalMap(Texture * _texture)
{
	if (nullptr == _texture)
	{
		return;
	}

	if (nullptr == mSrvDescriptorHeap)
	{
		CreateShaderResourceHeapAndView();
	}

	mNormalMap = _texture;
	mNormalMapFlag = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(1, D3DApplication::Instance()->GetCbvSrvDescriptorSize());
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mNormalMap->Get()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mNormalMap->Get()->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	D3DApplication::Instance()->Device()->CreateShaderResourceView(mNormalMap->Get(), &srvDesc, hDescriptor);
}

void Material::AttachSpecularMap(Texture * _texture)
{
	if (nullptr == _texture)
	{
		return;
	}

	if (nullptr == mSrvDescriptorHeap)
	{
		CreateShaderResourceHeapAndView();
	}

	mSpecularMap = _texture;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	hDescriptor.Offset(2, D3DApplication::Instance()->GetCbvSrvDescriptorSize());

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mSpecularMap->Get()->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mSpecularMap->Get()->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	D3DApplication::Instance()->Device()->CreateShaderResourceView(mSpecularMap->Get(), &srvDesc, hDescriptor);
}

void Material::_AWAKE_()
{
}

void Material::_UPDATE_(const float _deltaTime)
{
}

void Material::_DRAW_(const float _deltaTime)
{
}

void Material::_DESTORY_()
{
}