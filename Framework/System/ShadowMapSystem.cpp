#include "ShadowMapSystem.h"
#include "D3DApplication.h"

ShadowMapSystem * ShadowMapSystem::Instance()
{
	static ShadowMapSystem instance;
	return &instance;
}

void ShadowMapSystem::Initialize(UINT _width, UINT _height)
{
	mWidth = _width;
	mHeight = _height;

	mViewPort = { 0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height), 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)_width, (int)_height };
	
	CreateResource();
	CreateDescriptorHeap();
	CreateResourceView();
}

void ShadowMapSystem::Release()
{

}

void ShadowMapSystem::OnResize(UINT _width, UINT _height)
{
	if ((mWidth != _width) || (mHeight != _height))
	{
		mWidth = _width;
		mHeight = _height;

		CreateResource();

		// �� Resource�̹Ƿ� ���ο� Descriptor�� �ʿ��ϴ�.
		CreateResourceView();
	}
}

void ShadowMapSystem::BindDescriptorTable()
{
	// Descriptor Heap�� ���������ο� ���ε��Ѵ�.
	ID3D12DescriptorHeap* descriptorHeaps[] = { GetSrvDescriptorHeap().Get() };
	D3DApplication::Instance()->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Shader Resource View�� ���̴��� ���ε��Ѵ�.
	// �� �н������� �ش� Texture�� ���� ���� �ۼ��ϰ�
	// ���� �н����� �ش� �ؽ�ó�� ����, �׸��ڸ� ����Ͽ� ������ ��.
	D3DApplication::Instance()->CommandList()->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_ID::SHADOW_MAP, SRV());
}

UINT ShadowMapSystem::Width() const
{
	return mWidth;
}

UINT ShadowMapSystem::Height() const
{
	return mHeight;
}

D3D12_VIEWPORT ShadowMapSystem::ViewPort() const
{
	return mViewPort;
}

D3D12_RECT ShadowMapSystem::ScissorRect() const
{
	return mScissorRect;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> ShadowMapSystem::GetSrvDescriptorHeap() const
{
	return mSrvDescriptorHeap;
}

ID3D12Resource* ShadowMapSystem::Resource()
{
	return mShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMapSystem::SRV() const
{
	return mGpuSrvHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMapSystem::DSV() const
{
	return mCpuDsvHandle;
}

void ShadowMapSystem::CreateResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(D3DApplication::Instance()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}

void ShadowMapSystem::CreateResourceView()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();
	UINT cbvSrvDescSize = D3DApplication::Instance()->GetCbvSrvDescriptorSize();

	// Shader Resource View
	mCpuSrvHandle = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// GPU Shader Resource View�� Null�� ����.(GPU�� �Ѱ��� ������ �����Ƿ�)
	mGpuSrvHandle = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCpuSrvHandle.Offset(0, cbvSrvDescSize);

	// ���̴����� �׸��� ���� ���ø� �� �� �ֵ��� Resource�� ���� SRV�� ������, Shader Resource ������ ���� ���ε�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	device->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mCpuSrvHandle);

	// Depth Shader Resource View
	mCpuDsvHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mCpuDsvHandle.Offset(0, D3DApplication::Instance()->GetCbvSrvDescriptorSize());

	// �׸��� �ʿ� ������ �� �� �ֵ��� Resource�� DSV�� �����Ͽ�, Depth Stencil Resource ������ ���� ���ε�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mCpuDsvHandle);
}

void ShadowMapSystem::CreateDescriptorHeap()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	// ������ Heap�� �����ϱ� ����, Descriptor ����ü�� ����
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1; // ���� ���� Shadow Map�� ������ �� �ִ�.
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// ������ Heap ����
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvDescriptorHeap.GetAddressOf())));
}
