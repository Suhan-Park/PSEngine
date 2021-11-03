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

	// 1. Device 객체를 생성한다.
	CreateDevice();

	// 2. Fence 객체를 생성한다.
	CreateFence();

	// 3. 명령 대기열, 할당자, 리스트를 생성한다.
	CreateCommandQueue();
	CreateCoomandAllocator();
	CreateCommandList();

	mPassCB = std::make_unique<UploadBuffer<PassConstants>>(md3dDevice.Get(), 2, true);
	mPassL = std::make_unique<UploadBuffer<PassLight>>(md3dDevice.Get(), 1, true);
	mSSAOCB = std::make_unique<UploadBuffer<SSAOConstants>>(md3dDevice.Get(), 1, true);

	// 초기화 명령 준비를 위해 명령 리스트를 재설정한다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 4. 멀티 샘플링 여부를 점검한다. (후면 버퍼 생성 전에 점검)
	Set4XMSAA();

	// 5. SwapChain을 생성한다.
	CreateSwapChain(_hWnd, mWidth, mHeight);

	// 6. 서술자의 크기를 얻어온다.
	SetDescriptorSize();

	// 7. Render Target 버퍼와 Depth Stencil 버퍼의 서술자의 힙을 생성한다.
	CreateRtvAndDsvHeap();

	// 8. RenderTarget View(서술자)를 생성한다.
	CreateRenderTargetView();

	// 9. Depth/Stencil View(서술자)와 버퍼를 생성한다.
	CreateDepthStencilBuffer(mWidth, mHeight);

	// 10. 뷰포트를 설정한다.
	SetViewport(mWidth, mHeight);

	// 11. 가위 직사각형을 설정한다.
	SetScissorRectangle(mWidth, mHeight);

	// 12. 루트 시그니처를 생성한다.
	CreateRootSignature();
	CreateSSAORootSignature();

	// 13. 쉐이더를 컴파일하고 빌드한다.
	BuildShader();

	// 14. 정점의 성분을 서술하는, 입력 배치 서술을 설정한다.
	SetInputLayout();

	// 15. PSO 객체를 생성한다.
	CreateGraphcisPipelineStateObject();

	// 16. Audio System 초기화
	AudioSystem::Instance()->Initialize();

	// 17. PhysX System 초기화
	PhysXSystem::Instance()->Initialize();

	// 18. Scene을 초기화한다.
	mScene = new DemoScene();
	mScene->_AWAKE_();

	// 19. Scene에 배치되는 카메라를 초기화한다.
	Camera::Main()->ResizeWindow(static_cast<float>(mWidth), static_cast<float>(mHeight));

	// 20. Shadow Map을 구성하기 위한 Manager를 초기화한다.
	ShadowMapSystem::Instance()->Initialize(2048, 2048);

	// 21. Sky Box 초기화
	SkyBoxSystem::Instance()->Initialize("Skybox", L"Textures/SkyBox4.dds");

	// 22. Screen Space Ambient Occlusion을 계산하기 위해 Manager를 초기화한다.
	SSAOSystem::Instance()->Initialize(mWidth, mHeight);
	SSAOSystem::Instance()->RegisterDescriptorsHandle(mDepthStencilBuffer.Get());
	SSAOSystem::Instance()->SetPSOs(mPSOs["SSAO"].Get(), mPSOs["SSAOBlur"].Get());

	// 초기화 명령을 실행하기 위해 명령 리스트를 닫는다.
	ThrowIfFailed(mCommandList->Close());

	// 초기화 명령을 실행한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 초기화가 끝날 때까지 대기한다.
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
	 * 윈도우가 WM_SIZE 메시지를 받으면, MsgProc가 이 메시지를 호출한다.
	 * 윈도우 크기가 변하면, (1)후면 버퍼와 (2)깊이, 스텐실 버퍼, (3)렌더 타겟 뷰와 스텐실 뷰도 다시 생성해야한다.
	 * 후면 버퍼의 크기는 IDXGISwapChain::ResizeBuffers로 변경할 수 있다.
	 */

	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// GPU가 이전의 요청들을 다 처리할 때 까지 기다린다.
	FlushCommandQueue();

	// 명령 할당자를 초기화한다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 이전 사용하고 있었던 자원을 해제한다.
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
	// 현재 Fence 지점까지의 명령들을 표시하도록 Fence 값을 증가시킨다.
	mCurrentFence++;

	// 새 Fence 지점을 설정하는 명령(Signal)을 명령 대기열에 추가한다.
	// 새 Fence 지점은 GPU가 Signal 명령까지의 모든 명령을 처리하기 전 까지는 설정되지 않는다.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// GPU가 이 Fence 지점 까지의 명령들을 완료할 때 까지 기다린다.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		// GPU가 현재 Fence 지점에 도달했으면, 이벤트를 발동한다.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// GPU가 현재 Fence 지점에 도달했음을 뜻하는 이벤트를 기다린다.
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
	// D3D12 디버그 레이어를 활성화한다.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// 하드웨어 어댑터를 나타내는 장치를 생성한다.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // 기본 어댑터
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// 생성에 실패했다면, WARP 어댑터를 나타내는 장치를 생성한다.
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
		mDirectCmdListAlloc.Get(), // 연관될 명령 할당자
		nullptr,                   // 초기 파이프라인 상태 객체
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// 이후 명령 목록을 처음에 참조할 때, Reset 함수를 호출 하는데, 닫힌 상태이어야 한다.
	// 그러므로, 닫힌 상태로 시작한다.
	mCommandList->Close();
}

void D3DApplication::Set4XMSAA()
{

	// 주어진 텍스처 형식과 표본 개수의 조합에 대해, 유효한 품질 수준은 0에서 NumQualityLevel-1까지이다.
	// 한 픽셀에서 추출할 수 있는 최대 표본 개수는 32개로 정의된다.
	// 보통은 메모리 비용의 관리를 위해 표본을 4개나 8개만 추출하는 경우가 많다.
	// 멀티샘플링을 사용하지 않으려면, 표본 개수(Count)를 1로 품질 수준(Quality)을 0으로 설정하면 된다.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 8;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	// 아래 함수는 둘째 매개변수로 지정된 구조체에서 텍스처 형식과 표본 개수를 읽고, 그에 해당하는
	// 품질 수준을 구조체의 NumQualityLevels 멤버에 설정한다. (msQualityLevels는 입출력에 모두 사용된다.)
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
}

void D3DApplication::CreateSwapChain(HWND _hWnd, int _width, int _height)
{
	// 기존 SwapChain을 해제한다.
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

	// SwapChain은 명령 대기열을 통해서 방출(Flush)를 수행한다.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApplication::ResizeSwapChain(const int _width, const int _height)
{
	// Swap Chain의 크기를 재설정한다.
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
		// SwapChain의 i번째 버퍼를 얻는다.
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));

		// 그 버퍼에 대한 렌더 타겟 서술자를 생성한다.
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

		// 힙의 다음 항목으로 넘어간다.
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}
}

void D3DApplication::CreateDepthStencilBuffer(const int _width, const int _height)
{
	// 깊이 스텐실 서술자를 생성한다.
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
	// 일반적으로 셰이더 프로그램은 특정 자원(상수 버퍼, 텍스처, 레스터라이저 등)이 입력된다고 기대된다.
	// 루트 시그니처는 셰이더 프로그램이 기대하는 자원들을 정의한다.
	// 셰이더 프로그램은 본질적으로 하나의 함수이고 셰이더에 입력되는 자원들은 함수의 매개변수들에 해당하므로,
	// 루트 시그니처는 함수에 대한 시그니처(서명)를 정의하는 수단이라고 할 수 있다.

	// CBV 하나를 담는 서술자 테이블을 생성한다.
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // 서술자 종류
		2,  // 서술자 개수. 예) 텍스처 개수
		0,  // 루트 매개변수에 묶일 쉐이더 인수의 기준 레지스터 번호. 예) 상수버퍼들은 b1,b2,b3로 지정된다. 텍스쳐는 서술자 두개 이므로 gTexture[2] : register(t0)
		0,  // 레지스터 공간. 예) 0번 레지스터 공간이므로 gTexture[2] : register(t0, space0)
		0); // 테이블 시작으로부터의 오프셋. 예) 테이블 하나에 서술자가 여러 개 있을 수 있으므로...

	// Shadow Map을 담는 서술자 테이블을 생성한다.
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // 서술자 종류
		1,
		0,
		1,
		0);

	CD3DX12_DESCRIPTOR_RANGE texTable2;
	texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // 서술자 종류
		1,
		0,
		2,
		0);

	// Cube Map을 담는 서술자 테이블을 생성한다.
	CD3DX12_DESCRIPTOR_RANGE texTable3;
	texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // 서술자 종류
		1,
		0,
		3,
		0);

	// 루트 매개변수는 서술자 테이블이거나 루트 서술자 혹은 루트 상수이다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[9];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	slotRootParameter[1].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // Shadow Map
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL); // SSAO Map
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL); // Cube Map
	slotRootParameter[4].InitAsConstantBufferView(0); // 레지스터 b0 = Object Constant
	slotRootParameter[5].InitAsConstantBufferView(1); // 레지스터 b1 = Pass Constat
	slotRootParameter[6].InitAsConstantBufferView(2); // 레지스터 b2 = Light
	slotRootParameter[7].InitAsConstantBufferView(3); // 레지스터 b3 = Material Data
	slotRootParameter[8].InitAsConstantBufferView(4); // 레지스터 b4 = Skinned Constant

	auto staticSamplers = GetStaticSamplers();

	// 루트 서명은 루트 매개변수들의 배열이다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(9, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 루트 시그니처를 생성한다.
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

	// 카메라가 Sky Box안에 있으므로, Culling 기능을 끈다.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// 깊이 함수가 LESS_EQUAL 확인.
	// 그렇지 않으면, 깊이 버퍼가 1로 지워진 경우, NDC 좌표에서 z = 1 일 때, 깊이 테스트에 실패한다.
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

	// Shadow Map Pass에서는 Render Target이 필요하지 않음.
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

	// Pass 상수 버퍼에서도 사용하므로, 예외적으로 해당 블록에서 갱신한다
	XMStoreFloat4x4(&mPassConstants.Shadow, XMMatrixTranspose(S));

	UINT w = ShadowMapSystem::Instance()->Width();
	UINT h = ShadowMapSystem::Instance()->Height();

	mShadowPassConstants.EyePosW = LightSystem::Instance()->GetMainDirectionalLight()->LightPosition();
	mShadowPassConstants.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassConstants.InvRenderTargetSize = XMFLOAT2((float)1.0f / w, (float)1.0f / h);
	mShadowPassConstants.NearZ = LightSystem::Instance()->GetMainDirectionalLight()->NearZ();
	mShadowPassConstants.FarZ = LightSystem::Instance()->GetMainDirectionalLight()->FarZ();

	// 파이프라인으로 넘기기 위해 버퍼에 저장한다. (1번 인덱스에 저장)
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

	// 렌더 타겟을 변경한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 화면의 Normal Map과 Depth Buffer를 지운다.
	float clearValue[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 렌더링을 진행할 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

	// 해당 Pass에서 사용할 상수 버퍼를 지정한다.
	auto passCB = mPassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["DrawNormals"].Get());

	mScene->_DRAW_(_deltaTime);

	// 자원을 쉐이더에서 읽을 수 있도록, GENERIC_READ 상태로 바꾼다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void D3DApplication::_UPDATE_(const float _deltaTime, const float _totalTime)
{
	// Mesh Render에서 Object Constant Buffer 갱신
	// Material Data는 Material Data 값이 변경될 때 갱신(Albedo, ...) - 성능 상 이유로
	mScene->_UPDATE_(_deltaTime);

	// Shadow Constant Buffer 갱신
	UpdateShadowPassConstantBuffer();

	// Pass Constant Buffer 갱신
	UpdateMainPassConstantBuffer(mWidth, mHeight, _deltaTime, _totalTime);

	// SSAO Constant Buffer 갱신
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
	// 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
	// 재설정은 GPU가 관련 명력 리스트들을 모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 명령 리스트를 ExecuteCommandList를 통해서 명령 대기열에 추가했다면,
	// 명령 목록을 재설정할 수 있다. 명령 목록을 재설정하면 메모리가 재활용된다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs["Standard"].Get()));

	// 루트 서명을 지정한다.
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

#pragma region Shadow Map Render Pass

	ShadowMapSystem* shadowMapSystem = ShadowMapSystem::Instance();

	mCommandList->RSSetViewports(1, &shadowMapSystem->ViewPort());
	mCommandList->RSSetScissorRects(1, &shadowMapSystem->ScissorRect());

	// DEPTH_WRITE 상태로 전이 상태 설정
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMapSystem->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 후면 버퍼와 깊이 버퍼를 지운다.
	mCommandList->ClearDepthStencilView(shadowMapSystem->DSV(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 렌더 타겟을 nullptr로 설정한다. (깊이 버퍼에 그릴 것이므로.)
	// nullptr로 설정하면, 색상을 기록하는 것을 비활성화 한다.
	// PSO의 Render Target은 0으로 설정되어야 한다.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &shadowMapSystem->DSV());

	UINT passCBByteSize = CalcConstantBufferByteSize(sizeof(PassConstants));

	// Pass 정보를 쉐이더로 넘긴다. (Shadow Pass이므로 Offset 만큼 이동한다.)
	auto passCB = mPassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (1 * passCBByteSize);
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCBAddress);

	// Shadow Map을 구성하기 위해, 서술자 테이블을 바인딩한다.
	shadowMapSystem->BindDescriptorTable();

	// PSO를 설정한다.
	mCommandList->SetPipelineState(mPSOs["SkinnedShadowMap"].Get());

	// 그린다.
	mScene->_DRAW_(_deltaTime);

	// 자원을 쉐이더에서 읽을 수 있도록, GENERIC_READ 상태로 바꾼다.
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

	// 뷰포트와 가위 직사각형을 설정한다.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 후면 버퍼와 깊이 버퍼를 지운다.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 렌더링 결과가 기록될 렌더 타겟 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Pass 정보를 쉐이더로 넘긴다. (2번 항목 - b1 register)
	passCB = mPassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::PASS_CONSTANT, passCB->GetGPUVirtualAddress());

	auto passL = mPassL->Resource();
	mCommandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::LIGHT, passL->GetGPUVirtualAddress());

	ShadowMapSystem::Instance()->BindDescriptorTable();

	SSAOSystem::Instance()->BindDescriptorsTable();

	mCommandList->SetPipelineState(mPSOs["Standard"].Get());

	// 그린다.
	mScene->_DRAW_(_deltaTime);

#pragma endregion

#pragma region Sky Box

	mCommandList->SetPipelineState(mPSOs["Sky"].Get());
	SkyBoxSystem::Instance()->Draw();

#pragma endregion

	// 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// 명령 기록을 마친다.
	ThrowIfFailed(mCommandList->Close());

	// 명령 실행을 위해 명령 리스트를 명령 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 후면 버퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 이 프레임의 명령들이 모두 처리되길 기다린다.
	// 이러한 대기는 비효율적이다.
	FlushCommandQueue();
}

// ID3D12Device::RemoveDevice: Device removal has been triggered for the following reason (DXGI_ERROR_DEVICE_HUNG)
// 다음 에러 발생시, 루트 서명이 제대로 이루어졌는지 확인.