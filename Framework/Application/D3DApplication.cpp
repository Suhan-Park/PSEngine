#include "D3DApplication.h"
#include "Scene.h"
#include "DemoScene.h"

#include "SSAOSystem.h"
#include "LightSystem.h"
#include "ShadowMapSystem.h"
#include "SkyBoxSystem.h"
#include "AudioSystem.h"
#include "PhysXSystem.h"

#include "Light.h"
#include "Texture.h"
#include "Camera.h"
#include "Transform.h"

using namespace DirectX;

D3DApplication::~D3DApplication()
{

}

D3DApplication* D3DApplication::Instance()
{
	static D3DApplication d3dApp;
	return &d3dApp;
}

bool D3DApplication::Initialize(const HWND _hWnd, const int _width, const int _height)
{
	mWidth = _width;
	mHeight = _height;

	// 1. Device ��ü�� �����Ѵ�.
	CreateDevice();

	// 2. Fence ��ü�� �����Ѵ�.
	CreateFence();

	// 3. ��� ��⿭, �Ҵ���, ����Ʈ�� �����Ѵ�.
	CreateCommandQueue();
	CreateCoomandAllocator();
	CreateCommandList();

	mPassCB = std::make_unique<UploadBuffer<PassConstants>>(md3dDevice.Get(), 2, true);
	mPassL = std::make_unique<UploadBuffer<PassLight>>(md3dDevice.Get(), 1, true);
	mSSAOCB = std::make_unique<UploadBuffer<SSAOConstants>>(md3dDevice.Get(), 1, true);

	// �ʱ�ȭ ��� �غ� ���� ��� ����Ʈ�� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 4. ��Ƽ ���ø� ���θ� �����Ѵ�. (�ĸ� ���� ���� ���� ����)
	Set4XMSAA();

	// 5. SwapChain�� �����Ѵ�.
	CreateSwapChain(_hWnd, mWidth, mHeight);

	// 6. �������� ũ�⸦ ���´�.
	SetDescriptorSize();

	// 7. Render Target ���ۿ� Depth Stencil ������ �������� ���� �����Ѵ�.
	CreateRtvAndDsvHeap();

	// 8. RenderTarget View(������)�� �����Ѵ�.
	CreateRenderTargetView();

	// 9. Depth/Stencil View(������)�� ���۸� �����Ѵ�.
	CreateDepthStencilBuffer(mWidth, mHeight);

	// 10. ����Ʈ�� �����Ѵ�.
	SetViewport(mWidth, mHeight);

	// 11. ���� ���簢���� �����Ѵ�.
	SetScissorRectangle(mWidth, mHeight);

	// 12. ��Ʈ �ñ״�ó�� �����Ѵ�.
	CreateRootSignature();
	CreateSSAORootSignature();

	// 13. ���̴��� �������ϰ� �����Ѵ�.
	BuildShader();

	// 14. ������ ������ �����ϴ�, �Է� ��ġ ������ �����Ѵ�.
	SetInputLayout();

	// 15. PSO ��ü�� �����Ѵ�.
	CreateGraphcisPipelineStateObject();

	// 16. Audio System �ʱ�ȭ
	AudioSystem::Instance()->Initialize();

	// 17. PhysX System �ʱ�ȭ
	PhysXSystem::Instance()->Initialize();

	// 18. Scene�� �ʱ�ȭ�Ѵ�.
	mScene = new DemoScene();
	mScene->_AWAKE_();

	// 19. Scene�� ��ġ�Ǵ� ī�޶� �ʱ�ȭ�Ѵ�.
	Camera::Main()->ResizeWindow(static_cast<float>(mWidth), static_cast<float>(mHeight));

	// 20. Shadow Map�� �����ϱ� ���� Manager�� �ʱ�ȭ�Ѵ�.
	ShadowMapSystem::Instance()->Initialize(2048, 2048);

	// 21. Sky Box �ʱ�ȭ
	SkyBoxSystem::Instance()->Initialize("Skybox", L"Textures/SkyBox4.dds");

	// 22. Screen Space Ambient Occlusion�� ����ϱ� ���� Manager�� �ʱ�ȭ�Ѵ�.
	SSAOSystem::Instance()->Initialize(mWidth, mHeight);
	SSAOSystem::Instance()->RegisterDescriptorsHandle(mDepthStencilBuffer.Get());
	SSAOSystem::Instance()->SetPSOs(mPSOs["SSAO"].Get(), mPSOs["SSAOBlur"].Get());

	// �ʱ�ȭ ����� �����ϱ� ���� ��� ����Ʈ�� �ݴ´�.
	ThrowIfFailed(mCommandList->Close());

	// �ʱ�ȭ ����� �����Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ�� ���� ������ ����Ѵ�.
	FlushCommandQueue();

	return true;
}

void D3DApplication::Run(const Timer& _timer)
{
	const float deltaTime = _timer.DeltaTime();
	const float totalTime = _timer.TotalTime();

	mAccumulatedTime += deltaTime;

	_UPDATE_(deltaTime, totalTime);

	if (mAccumulatedTime >= 0.01f)
	{
		_FIXED_UPDATE_(0.01f, totalTime);
		mAccumulatedTime = 0.0f;
	}
	_DRAW_(deltaTime, totalTime);
}

void D3DApplication::Quit()
{
	LightSystem::Instance()->Release();
	ShadowMapSystem::Instance()->Release();
	AudioSystem::Instance()->Release();
	PhysXSystem::Instance()->Release();

	mScene->_DESTORY_();
}

void D3DApplication::OnResize(const HWND _hWnd, const int _width, const int _height)
{
	/*
	 * �����찡 WM_SIZE �޽����� ������, MsgProc�� �� �޽����� ȣ���Ѵ�.
	 * ������ ũ�Ⱑ ���ϸ�, (1)�ĸ� ���ۿ� (2)����, ���ٽ� ����, (3)���� Ÿ�� ��� ���ٽ� �䵵 �ٽ� �����ؾ��Ѵ�.
	 * �ĸ� ������ ũ��� IDXGISwapChain::ResizeBuffers�� ������ �� �ִ�.
	 */

	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// GPU�� ������ ��û���� �� ó���� �� ���� ��ٸ���.
	FlushCommandQueue();

	// ��� �Ҵ��ڸ� �ʱ�ȭ�Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// ���� ����ϰ� �־��� �ڿ��� �����Ѵ�.
	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		mSwapChainBuffer[i].Reset();
	}

	mDepthStencilBuffer.Reset();

	ResizeSwapChain(_width, _height);
	CreateRenderTargetView();
	CreateDepthStencilBuffer(_width, _height);

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	Camera::Main()->ResizeWindow(static_cast<float>(_width), static_cast<float>(_height));
	SetViewport(_width, _height);
	SetScissorRectangle(_width, _height);

	SSAOSystem::Instance()->OnResize(mWidth, mHeight);
	SSAOSystem::Instance()->CreateDescriptors(mDepthStencilBuffer.Get());
}

void D3DApplication::FlushCommandQueue()
{
	// ���� Fence ���������� ��ɵ��� ǥ���ϵ��� Fence ���� ������Ų��.
	mCurrentFence++;

	// �� Fence ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� Fence ������ GPU�� Signal ��ɱ����� ��� ����� ó���ϱ� �� ������ �������� �ʴ´�.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// GPU�� �� Fence ���� ������ ��ɵ��� �Ϸ��� �� ���� ��ٸ���.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		// GPU�� ���� Fence ������ ����������, �̺�Ʈ�� �ߵ��Ѵ�.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// GPU�� ���� Fence ������ ���������� ���ϴ� �̺�Ʈ�� ��ٸ���.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

Microsoft::WRL::ComPtr<ID3D12Device> D3DApplication::Device() const
{
	return md3dDevice;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> D3DApplication::CommandList() const
{
	return mCommandList;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3DApplication::CommandAllocator() const
{
	return mDirectCmdListAlloc;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3DApplication::CommandQueue() const
{
	return mCommandQueue;
}

ID3D12Resource* D3DApplication::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApplication::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApplication::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApplication::CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// D3D12 ����� ���̾ Ȱ��ȭ�Ѵ�.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// �ϵ���� ����͸� ��Ÿ���� ��ġ�� �����Ѵ�.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // �⺻ �����
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// ������ �����ߴٸ�, WARP ����͸� ��Ÿ���� ��ġ�� �����Ѵ�.
	if (FAILED(hardwareResult))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}
}

void D3DApplication::CreateFence()
{
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));
}

void D3DApplication::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

void D3DApplication::CreateCoomandAllocator()
{
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
}

void D3DApplication::CreateCommandList()
{
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // ������ ��� �Ҵ���
		nullptr,                   // �ʱ� ���������� ���� ��ü
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// ���� ��� ����� ó���� ������ ��, Reset �Լ��� ȣ�� �ϴµ�, ���� �����̾�� �Ѵ�.
	// �׷��Ƿ�, ���� ���·� �����Ѵ�.
	mCommandList->Close();
}

void D3DApplication::Set4XMSAA()
{

	// �־��� �ؽ�ó ���İ� ǥ�� ������ ���տ� ����, ��ȿ�� ǰ�� ������ 0���� NumQualityLevel-1�����̴�.
	// �� �ȼ����� ������ �� �ִ� �ִ� ǥ�� ������ 32���� ���ǵȴ�.
	// ������ �޸� ����� ������ ���� ǥ���� 4���� 8���� �����ϴ� ��찡 ����.
	// ��Ƽ���ø��� ������� ��������, ǥ�� ����(Count)�� 1�� ǰ�� ����(Quality)�� 0���� �����ϸ� �ȴ�.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 8;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	// �Ʒ� �Լ��� ��° �Ű������� ������ ����ü���� �ؽ�ó ���İ� ǥ�� ������ �а�, �׿� �ش��ϴ�
	// ǰ�� ������ ����ü�� NumQualityLevels ����� �����Ѵ�. (msQualityLevels�� ����¿� ��� ���ȴ�.)
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
}

void D3DApplication::CreateSwapChain(HWND _hWnd, int _width, int _height)
{
	// ���� SwapChain�� �����Ѵ�.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = _width;
	sd.BufferDesc.Height = _height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = _hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// SwapChain�� ��� ��⿭�� ���ؼ� ����(Flush)�� �����Ѵ�.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApplication::ResizeSwapChain(const int _width, const int _height)
{
	// Swap Chain�� ũ�⸦ �缳���Ѵ�.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		_width, _height,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
}

void D3DApplication::SetDescriptorSize()
{
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DApplication::CreateRtvAndDsvHeap()
{
	CreateRenderTagetViewHeap();
	CreateDepthStencilViewHeap();
}

void D3DApplication::CreateRenderTagetViewHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
}

void D3DApplication::CreateRenderTargetView()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// SwapChain�� i��° ���۸� ��´�.
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));

		// �� ���ۿ� ���� ���� Ÿ�� �����ڸ� �����Ѵ�.
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

		// ���� ���� �׸����� �Ѿ��.
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}
}

void D3DApplication::CreateDepthStencilBuffer(const int _width, const int _height)
{
	// ���� ���ٽ� �����ڸ� �����Ѵ�.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = _width;
	depthStencilDesc.Height = _height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void D3DApplication::CreateDepthStencilViewHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApplication::SetViewport(const int _width, const int _height)
{
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(_width);
	mScreenViewport.Height = static_cast<float>(_height);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;
}

void D3DApplication::SetScissorRectangle(const int _width, const int _height)
{
	mScissorRect = { 0, 0, _width, _height };
}

void D3DApplication::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO skinnedDefines[] =
	{
		"SKINNED", "1",
		NULL, NULL
	};

	mShaders["StandardVS"] = CompileShader(L"Shader\\color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkinnedStandardVS"] = CompileShader(L"Shader\\color.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["StandardPS"] = CompileShader(L"Shader\\color.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ShadowVS"] = CompileShader(L"Shader\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkinnedShadowVS"] = CompileShader(L"Shader\\Shadows.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["ShadowPS"] = CompileShader(L"Shader\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["ShadowPSAlphaTest"] = CompileShader(L"Shader\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["debugVS"] = CompileShader(L"Shader\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["debugPS"] = CompileShader(L"Shader\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkyVS"] = CompileShader(L"Shader\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyPS"] = CompileShader(L"Shader\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["DrawNormalsVS"] = CompileShader(L"Shader\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkinnedDrawNormalsVS"] = CompileShader(L"Shader\\DrawNormals.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["DrawNormalsPS"] = CompileShader(L"Shader\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SSAOVS"] = CompileShader(L"Shader\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SSAOPS"] = CompileShader(L"Shader\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SSAOBlurVS"] = CompileShader(L"Shader\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SSAOBlurPS"] = CompileShader(L"Shader\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");
}

void D3DApplication::SetInputLayout()
{
	HRESULT hr = S_OK;

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R32G32B32_UINT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mAnimationInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R32G32B32_UINT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void D3DApplication::CreateRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ�(��� ����, �ؽ�ó, �����Ͷ����� ��)�� �Էµȴٰ� ���ȴ�.
	// ��Ʈ �ñ״�ó�� ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.
	// ���̴� ���α׷��� ���������� �ϳ��� �Լ��̰� ���̴��� �ԷµǴ� �ڿ����� �Լ��� �Ű������鿡 �ش��ϹǷ�,
	// ��Ʈ �ñ״�ó�� �Լ��� ���� �ñ״�ó(����)�� �����ϴ� �����̶�� �� �� �ִ�.

	// CBV �ϳ��� ��� ������ ���̺��� �����Ѵ�.
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // ������ ����
		2,  // ������ ����. ��) �ؽ�ó ����
		0,  // ��Ʈ �Ű������� ���� ���̴� �μ��� ���� �������� ��ȣ. ��) ������۵��� b1,b2,b3�� �����ȴ�. �ؽ��Ĵ� ������ �ΰ� �̹Ƿ� gTexture[2] : register(t0)
		0,  // �������� ����. ��) 0�� �������� �����̹Ƿ� gTexture[2] : register(t0, space0)
		0); // ���̺� �������κ����� ������. ��) ���̺� �ϳ��� �����ڰ� ���� �� ���� �� �����Ƿ�...

	// Shadow Map�� ��� ������ ���̺��� �����Ѵ�.
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // ������ ����
		1,
		0,
		1,
		0);

	CD3DX12_DESCRIPTOR_RANGE texTable2;
	texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // ������ ����
		1,
		0,
		2,
		0);

	// Cube Map�� ��� ������ ���̺��� �����Ѵ�.
	CD3DX12_DESCRIPTOR_RANGE texTable3;
	texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // ������ ����
		1,
		0,
		3,
		0);

	// ��Ʈ �Ű������� ������ ���̺��̰ų� ��Ʈ ������ Ȥ�� ��Ʈ ����̴�.
	CD3DX12_ROOT_PARAMETER slotRootParameter[9];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	slotRootParameter[1].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // Shadow Map
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL); // SSAO Map
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL); // Cube Map
	slotRootParameter[4].InitAsConstantBufferView(0); // �������� b0 = Object Constant
	slotRootParameter[5].InitAsConstantBufferView(1); // �������� b1 = Pass Constat
	slotRootParameter[6].InitAsConstantBufferView(2); // �������� b2 = Light
	slotRootParameter[7].InitAsConstantBufferView(3); // �������� b3 = Material Data
	slotRootParameter[8].InitAsConstantBufferView(4); // �������� b4 = Skinned Constant

	auto staticSamplers = GetStaticSamplers();

	// ��Ʈ ������ ��Ʈ �Ű��������� �迭�̴�.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(9, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// ��Ʈ �ñ״�ó�� �����Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void D3DApplication::CreateSSAORootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1, 1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mSSAORootSignature.GetAddressOf())));
}

void D3DApplication::CreateGraphcisPipelineStateObject()
{
#pragma region Standard PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["StandardVS"]->GetBufferPointer()),
		mShaders["StandardVS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["StandardPS"]->GetBufferPointer()),
		mShaders["StandardPS"]->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = psoDesc;
	opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["Standard"])));

	// Standard Animation PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC animatedOpaquePsoDesc = opaquePsoDesc;
	animatedOpaquePsoDesc.InputLayout = { mAnimationInputLayout.data(), (UINT)mAnimationInputLayout.size() };
	animatedOpaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkinnedStandardVS"]->GetBufferPointer()),
		mShaders["SkinnedStandardVS"]->GetBufferSize()
	};
	animatedOpaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["StandardPS"]->GetBufferPointer()),
		mShaders["StandardPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&animatedOpaquePsoDesc, IID_PPV_ARGS(&mPSOs["SkinnedStandard"])));

#pragma endregion

#pragma region SkyBox PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = psoDesc;

	// ī�޶� Sky Box�ȿ� �����Ƿ�, Culling ����� ����.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// ���� �Լ��� LESS_EQUAL Ȯ��.
	// �׷��� ������, ���� ���۰� 1�� ������ ���, NDC ��ǥ���� z = 1 �� ��, ���� �׽�Ʈ�� �����Ѵ�.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = mRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyVS"]->GetBufferPointer()),
		mShaders["SkyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyPS"]->GetBufferPointer()),
		mShaders["SkyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["Sky"])));

#pragma endregion

#pragma region ShadowMap PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = psoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = mRootSignature.Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowVS"]->GetBufferPointer()),
		mShaders["ShadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowPS"]->GetBufferPointer()),
		mShaders["ShadowPS"]->GetBufferSize()
	};

	// Shadow Map Pass������ Render Target�� �ʿ����� ����.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["ShadowMap"])));

	// Animation Shadow PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC animatedSmapPsoDesc = smapPsoDesc;
	animatedSmapPsoDesc.InputLayout = { mAnimationInputLayout.data(), (UINT)mAnimationInputLayout.size() };
	animatedSmapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkinnedShadowVS"]->GetBufferPointer()),
		mShaders["SkinnedShadowVS"]->GetBufferSize()
	};
	animatedSmapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowPS"]->GetBufferPointer()),
		mShaders["ShadowPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&animatedSmapPsoDesc, IID_PPV_ARGS(&mPSOs["SkinnedShadowMap"])));
#pragma endregion

#pragma region Draw Normal PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = psoDesc;
	drawNormalsPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["DrawNormalsVS"]->GetBufferPointer()),
		mShaders["DrawNormalsVS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DrawNormalsPS"]->GetBufferPointer()),
		mShaders["DrawNormalsPS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.RTVFormats[0] = SSAOSystem::NormalMapFormat;
	drawNormalsPsoDesc.SampleDesc.Count = 1;
	drawNormalsPsoDesc.SampleDesc.Quality = 0;
	drawNormalsPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["DrawNormals"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC animatedDrawNormalsPsoDesc = drawNormalsPsoDesc;
	animatedDrawNormalsPsoDesc.InputLayout = { mAnimationInputLayout.data(), (UINT)mAnimationInputLayout.size() };
	animatedDrawNormalsPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkinnedDrawNormalsVS"]->GetBufferPointer()),
		mShaders["SkinnedDrawNormalsVS"]->GetBufferSize()
	};
	animatedDrawNormalsPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DrawNormalsPS"]->GetBufferPointer()),
		mShaders["DrawNormalsPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&animatedDrawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["SkinnedDrawNormals"])));
#pragma endregion

#pragma region SSAO PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = psoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSSAORootSignature.Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SSAOVS"]->GetBufferPointer()),
		mShaders["SSAOVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SSAOPS"]->GetBufferPointer()),
		mShaders["SSAOPS"]->GetBufferSize()
	};

	// SSAO effect does not need the depth buffer.
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = SSAOSystem::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["SSAO"])));
#pragma endregion

#pragma region SSAO Blur PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SSAOBlurVS"]->GetBufferPointer()),
		mShaders["SSAOBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SSAOBlurPS"]->GetBufferPointer()),
		mShaders["SSAOBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["SSAOBlur"])));

#pragma endregion

#pragma region Debug PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = psoDesc;
	debugPsoDesc.pRootSignature = mRootSignature.Get();
	debugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
		mShaders["debugVS"]->GetBufferSize()
	};
	debugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
		mShaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["debug"])));

#pragma endregion
}

void D3DApplication::UpdateMainPassConstantBuffer(const int _width, const int _height, const float _deltaTime, const float _totalTime)
{
	XMMATRIX view = Camera::Main()->ViewMatrix();
	XMMATRIX proj = Camera::Main()->ProjectionMatrix();
	XMMATRIX S = LightSystem::Instance()->GetMainDirectionalLight()->ShadowTransformMatrix();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invproj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invProjView = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);

	XMStoreFloat4x4(&mPassConstants.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mPassConstants.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mPassConstants.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mPassConstants.InvProj, XMMatrixTranspose(invproj));
	XMStoreFloat4x4(&mPassConstants.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mPassConstants.InvViewProj, XMMatrixTranspose(invProjView));
	XMStoreFloat4x4(&mPassConstants.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat4x4(&mPassConstants.Shadow, XMMatrixTranspose(S));

	mPassConstants.EyePosW = Camera::Main()->GetComponent<Transform>()->GetWorldPosition();
	mPassConstants.AmbientLight = XMFLOAT4(0.15f, 0.24f, 0.18f, 1.0f);
	mPassConstants.RenderTargetSize = XMFLOAT2((float)_width, (float)_height);
	mPassConstants.InvRenderTargetSize = XMFLOAT2((float)1.0f / _width, (float)1.0f / _height);
	mPassConstants.NearZ = Camera::Main()->NearZ();
	mPassConstants.FarZ = Camera::Main()->FarZ();
	mPassConstants.DeltaTime = _deltaTime;
	mPassConstants.TotalTime = _totalTime;

	mPassLight.DirectionalLightCount = LightSystem::Instance()->GetDirectionalLightCount();
	mPassLight.PointLightCount = LightSystem::Instance()->GetPointLightCount();
	mPassLight.SpotLightCount = LightSystem::Instance()->GetSpotLightCount();
	LightSystem::Instance()->GetLightData(mPassLight.Light, 15);

	auto curPassCB = mPassCB.get();
	curPassCB->CopyData(0, mPassConstants);

	auto curPassL = mPassL.get();
	curPassL->CopyData(0, mPassLight);
}

void D3DApplication::UpdateShadowPassConstantBuffer()
{
	XMMATRIX view = LightSystem::Instance()->GetMainDirectionalLight()->ViewMatrix();
	XMMATRIX proj = LightSystem::Instance()->GetMainDirectionalLight()->ProjectionMatrix();
	XMMATRIX S = LightSystem::Instance()->GetMainDirectionalLight()->ShadowTransformMatrix();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mShadowPassConstants.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassConstants.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassConstants.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassConstants.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassConstants.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassConstants.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mShadowPassConstants.Shadow, XMMatrixTranspose(S));

	// Pass ��� ���ۿ����� ����ϹǷ�, ���������� �ش� ��Ͽ��� �����Ѵ�
	XMStoreFloat4x4(&mPassConstants.Shadow, XMMatrixTranspose(S));

	UINT w = ShadowMapSystem::Instance()->Width();
	UINT h = ShadowMapSystem::Instance()->Height();

	mShadowPassConstants.EyePosW = LightSystem::Instance()->GetMainDirectionalLight()->LightPosition();
	mShadowPassConstants.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassConstants.InvRenderTargetSize = XMFLOAT2((float)1.0f / w, (float)1.0f / h);
	mShadowPassConstants.NearZ = LightSystem::Instance()->GetMainDirectionalLight()->NearZ();
	mShadowPassConstants.FarZ = LightSystem::Instance()->GetMainDirectionalLight()->FarZ();

	// �������������� �ѱ�� ���� ���ۿ� �����Ѵ�. (1�� �ε����� ����)
	auto currPassCB = mPassCB.get();
	currPassCB->CopyData(1, mShadowPassConstants);
}

void D3DApplication::UpdateSSAOConstantBuffer()
{
	XMMATRIX P = Camera::Main()->ProjectionMatrix();

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	mSSAOConstatnt.Proj = mPassConstants.Proj;
	mSSAOConstatnt.InvProj = mPassConstants.InvProj;
	XMStoreFloat4x4(&mSSAOConstatnt.ProjTex, XMMatrixTranspose(P*T));

	SSAOSystem::Instance()->GetOffsetVectors(mSSAOConstatnt.OffsetVectors);

	auto blurWeights = SSAOSystem::Instance()->CalcGaussWeights(2.5f);
	mSSAOConstatnt.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	mSSAOConstatnt.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	mSSAOConstatnt.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	mSSAOConstatnt.InvRenderTargetSize = XMFLOAT2(1.0f / SSAOSystem::Instance()->Width(), 1.0f / SSAOSystem::Instance()->Height());

	mSSAOConstatnt.OcclusionRadius = 0.5f;
	mSSAOConstatnt.OcclusionFadeStart = 0.2f;
	mSSAOConstatnt.OcclusionFadeEnd = 1.0f;
	mSSAOConstatnt.SurfaceEpsilon = 0.05f;

	auto currSsaoCB = mSSAOCB.get();
	currSsaoCB->CopyData(0, mSSAOConstatnt);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3DApplication::GetStaticSamplers()
{

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp ,
		shadow };
}

void D3DApplication::DrawNormalAndDepth(const float _deltaTime)
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto normalMap = SSAOSystem::Instance()->NormalMap();
	auto normalMapRtv = SSAOSystem::Instance()->NormalMapRtv();

	// ���� Ÿ���� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ȭ���� Normal Map�� Depth Buffer�� �����.
	float clearValue[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// �������� ������ ���۵��� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

	// �ش� Pass���� ����� ��� ���۸� �����Ѵ�.
	auto passCB = mPassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["DrawNormals"].Get());

	mScene->_DRAW_(_deltaTime);

	// �ڿ��� ���̴����� ���� �� �ֵ���, GENERIC_READ ���·� �ٲ۴�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void D3DApplication::_UPDATE_(const float _deltaTime, const float _totalTime)
{
	// Mesh Render���� Object Constant Buffer ����
	// Material Data�� Material Data ���� ����� �� ����(Albedo, ...) - ���� �� ������
	mScene->_UPDATE_(_deltaTime);

	// Shadow Constant Buffer ����
	UpdateShadowPassConstantBuffer();

	// Pass Constant Buffer ����
	UpdateMainPassConstantBuffer(mWidth, mHeight, _deltaTime, _totalTime);

	// SSAO Constant Buffer ����
	UpdateSSAOConstantBuffer();

	SkyBoxSystem::Instance()->Update();
	AudioSystem::Instance()->Update();
}

void D3DApplication::_FIXED_UPDATE_(const float _deltaTime, const float _totalTime)
{
	PhysXSystem::Instance()->Update(_deltaTime);
	mScene->_FIXED_UPDATE_(_deltaTime);
}

void D3DApplication::_DRAW_(const float _deltaTime, const float _totalTime)
{
	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��� ����Ʈ���� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// ��� ����Ʈ�� ExecuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�,
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs["Standard"].Get()));

	// ��Ʈ ������ �����Ѵ�.
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

#pragma region Shadow Map Render Pass

	ShadowMapSystem* shadowMapSystem = ShadowMapSystem::Instance();

	mCommandList->RSSetViewports(1, &shadowMapSystem->ViewPort());
	mCommandList->RSSetScissorRects(1, &shadowMapSystem->ScissorRect());

	// DEPTH_WRITE ���·� ���� ���� ����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMapSystem->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// �ĸ� ���ۿ� ���� ���۸� �����.
	mCommandList->ClearDepthStencilView(shadowMapSystem->DSV(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ���� Ÿ���� nullptr�� �����Ѵ�. (���� ���ۿ� �׸� ���̹Ƿ�.)
	// nullptr�� �����ϸ�, ������ ����ϴ� ���� ��Ȱ��ȭ �Ѵ�.
	// PSO�� Render Target�� 0���� �����Ǿ�� �Ѵ�.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &shadowMapSystem->DSV());

	UINT passCBByteSize = CalcConstantBufferByteSize(sizeof(PassConstants));

	// Pass ������ ���̴��� �ѱ��. (Shadow Pass�̹Ƿ� Offset ��ŭ �̵��Ѵ�.)
	auto passCB = mPassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (1 * passCBByteSize);
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCBAddress);

	// Shadow Map�� �����ϱ� ����, ������ ���̺��� ���ε��Ѵ�.
	shadowMapSystem->BindDescriptorTable();

	// PSO�� �����Ѵ�.
	mCommandList->SetPipelineState(mPSOs["SkinnedShadowMap"].Get());

	// �׸���.
	mScene->_DRAW_(_deltaTime);

	// �ڿ��� ���̴����� ���� �� �ֵ���, GENERIC_READ ���·� �ٲ۴�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMapSystem->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

#pragma endregion

#pragma region Normal Depth Render Pass

	DrawNormalAndDepth(_deltaTime);

#pragma endregion

#pragma region SSAO Pass

	mCommandList->SetGraphicsRootSignature(mSSAORootSignature.Get());
	SSAOSystem::Instance()->ComputeSSAO(3, mSSAOCB.get());

#pragma endregion

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

#pragma region Main Pass

	// ����Ʈ�� ���� ���簢���� �����Ѵ�.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �ĸ� ���ۿ� ���� ���۸� �����.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ������ ����� ��ϵ� ���� Ÿ�� ���۵��� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Pass ������ ���̴��� �ѱ��. (2�� �׸� - b1 register)
	passCB = mPassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCB->GetGPUVirtualAddress());

	auto passL = mPassL->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::LIGHT, passL->GetGPUVirtualAddress());

	ShadowMapSystem::Instance()->BindDescriptorTable();

	SSAOSystem::Instance()->BindDescriptorsTable();

	mCommandList->SetPipelineState(mPSOs["Standard"].Get());

	// �׸���.
	mScene->_DRAW_(_deltaTime);

#pragma endregion

#pragma region Sky Box

	mCommandList->SetPipelineState(mPSOs["Sky"].Get());
	SkyBoxSystem::Instance()->Draw();

#pragma endregion

	// �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// ��� ����� ��ģ��.
	ThrowIfFailed(mCommandList->Close());

	// ��� ������ ���� ��� ����Ʈ�� ��� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ĸ� ���ۿ� ���� ���۸� ��ȯ�Ѵ�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// �� �������� ��ɵ��� ��� ó���Ǳ� ��ٸ���.
	// �̷��� ���� ��ȿ�����̴�.
	FlushCommandQueue();
}

// ID3D12Device::RemoveDevice: Device removal has been triggered for the following reason (DXGI_ERROR_DEVICE_HUNG)
// ���� ���� �߻���, ��Ʈ ������ ����� �̷�������� Ȯ��.