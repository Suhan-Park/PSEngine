#ifndef _D3D_APPLICATION_H_
#define _D3D_APPLICATION_H_

#include "Application.h"

class Scene;

class D3DApplication
{
private:

	Scene* mScene = nullptr;

private:

	D3DApplication() = default;
	~D3DApplication();

	D3DApplication(const D3DApplication& _rhs) = delete;
	D3DApplication& operator= (const D3DApplication& _rhs) = delete;
	D3DApplication(D3DApplication&& _rhs) = delete;
	D3DApplication& operator= (D3DApplication&& _rhs) = delete;

public:

	static D3DApplication* Instance();

	bool Initialize(const HWND _hWnd, const int _width, const int _height);
	void Run(const Timer& _timer);
	void Quit();
	void OnResize(const HWND _hWnd, const int _width, const int _height);

	Microsoft::WRL::ComPtr<ID3D12Device> Device()const;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList()const;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator()const;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue()const;

	// �ζ��� ����� ����ο� ���Ǻ� ������, ���� ������ �߻��� �� �ִ�.
public:

	inline Scene* GetScene()
	{
		if (nullptr == mScene)
		{
			return nullptr;
		}
		
		return mScene;
	}

	inline UINT GetRtvDescriptorSize()
	{
		return mRtvDescriptorSize;
	}

	inline UINT GetDsvDescriptorSize()
	{
		return mDsvDescriptorSize;
	}

	inline UINT GetCbvSrvDescriptorSize()
	{
		return mCbvSrvUavDescriptorSize;
	}

	inline INT ApplicationWindowWidth()
	{
		return mWidth;
	}
	
	inline INT ApplicationWindowHeight()
	{
		return mHeight;
	}

private:

	/*
	 * : �⺻ �ʱ�ȭ
	 * 1. Device ��ü ����
	 * 2. CPU/GPU ����ȭ�� ���� Fence ��ü ����
	 * 3. ��� ��⿭,�Ҵ���,��� ����
	 * 4. ��Ƽ ���ø� ���� Ȯ�� �� SwapChain ����
	 * 5. ������ ũ�� ���ϰ� RTV/DSV �� ���� ����
	 * 6. ����Ʈ ����
	 * 7. ���� ���簢�� ����
	 */

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CreateDevice();

	void CreateFence();

	void CreateCommandQueue();
	void CreateCoomandAllocator();
	void CreateCommandList();

	void Set4XMSAA();
	void CreateSwapChain(const HWND _hWnd, const int _width, const int _height);
	void ResizeSwapChain(const int _width, const int _height);

	void SetDescriptorSize();
	void CreateRtvAndDsvHeap();
	void CreateRenderTagetViewHeap();
	void CreateDepthStencilViewHeap();

	void CreateRenderTargetView();
	void CreateDepthStencilBuffer(const int _width, const int _height);

	void SetViewport(const int _width, const int _height);
	void SetScissorRectangle(const int _width, const int _height);

	void BuildShader();
	void SetInputLayout();
	void CreateRootSignature();
	void CreateSSAORootSignature();
	void CreateGraphcisPipelineStateObject();
	
	void FlushCommandQueue();

	void UpdateMainPassConstantBuffer(const int _width, const int _height, const float _deltaTime, const float _totalTime);

	void UpdateShadowPassConstantBuffer();
	void UpdateSSAOConstantBuffer();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	void DrawNormalAndDepth(const float _deltaTime);

private:

	static const int SwapChainBufferCount = 2;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mSSAORootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mAnimationInputLayout;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence = nullptr;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount] = { nullptr };
	int mCurrBackBuffer = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	bool      m4xMsaaState = false;    // 4X MSAA enabled
	UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	PassConstants mPassConstants;
	PassConstants mShadowPassConstants;
	SSAOConstants mSSAOConstatnt;

	PassLight mPassLight;

	std::unique_ptr<UploadBuffer<PassConstants>> mPassCB = nullptr;
	std::unique_ptr<UploadBuffer<PassLight>> mPassL = nullptr;
	std::unique_ptr<UploadBuffer<SSAOConstants>> mSSAOCB = nullptr;

	INT mWidth;
	INT mHeight;

	float mAccumulatedTime = 0.0f;

private:

	void _UPDATE_(const float _deltaTime, const float _totalTime);
	void _FIXED_UPDATE_(const float _deltaTime, const float _totalTime);
	void _DRAW_(const float _deltaTime, const float _totalTime);

};

#endif // _D3D_APPLICATION_H_
