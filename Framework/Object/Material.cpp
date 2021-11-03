#include "Material.h"
#include "D3DApplication.h"

// Srv Heap�� ���ҽ��� �Ѱ��� �� �����ϰ�
// ��Ʈ ����(Draw�� ��)�� ���� ���Կ� ������ ���ε� �� �� �ִ�. (�� : 1�� ����, 2������ , ...)
// ��� ���ε� �� ��, ��ġ�� �˾ƾ���.
// �������, DiffuseMap, NormalMap, SpecularMap, CubeMap�� Srv Heap �� ���� �� �������� ��,
// 1�� Descriptor Table���� D, N, P�� ���ε��ϰ�, 2�� Descriptor Table���� Cube Map�� ���ε� �Ϸ��� ���� ��,
// Descriptor.Offset(CubeMapOffset, SrvDescriptorSize)��ŭ �о �ڵ��� �Ѱ��־�� �Ѵ�.
/*
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetGraphicsRootDescriptorTable(0, tex); // Diffuse, Normal, Specular Map
	Descriptor.Offset(CubeMapOffset, SrvDescriptorSize); // Offset��ŭ �̵�
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

	// Material�� ����Ǿ��� ��, Material ���۸� �����Ѵ�. (���̴������� ���켱�̹Ƿ�, ��ġ���ش�.)
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