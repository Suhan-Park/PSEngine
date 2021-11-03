#ifndef _SHADOW_MAP_SYSTEM_H_
#define _SHADOW_MAP_SYSTEM_H_

class ShadowMapSystem final
{
private:

	ShadowMapSystem() = default;
	~ShadowMapSystem() = default;

	ShadowMapSystem(const ShadowMapSystem& _rhs) = delete;
	ShadowMapSystem& operator = (const ShadowMapSystem& _rhs) = delete;
	ShadowMapSystem(ShadowMapSystem&& _rhs) = delete;
	ShadowMapSystem& operator = (ShadowMapSystem&& _rhs) = delete;

public:

	static ShadowMapSystem* Instance();

public:

	void Initialize(UINT _width, UINT _height);
	void Release();
	void OnResize(UINT _width, UINT _height);

	void BindDescriptorTable();

	UINT Width() const;
	UINT Height() const;

	D3D12_VIEWPORT ViewPort() const;
	D3D12_RECT ScissorRect() const;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() const;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE SRV() const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE DSV() const;

private:

	void CreateResource();
	void CreateResourceView();
	void CreateDescriptorHeap();

private:

	D3D12_VIEWPORT mViewPort;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuDsvHandle;

	// Light마다 적용한다면, 해당 변수를 컨테이너로 선언
	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;
};

#endif