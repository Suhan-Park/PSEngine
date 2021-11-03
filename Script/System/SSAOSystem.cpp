#include "SSAOSystem.h"

#include "D3DApplication.h"

using namespace PackedVector;

SSAOSystem * SSAOSystem::Instance()
{
	static SSAOSystem instance;
	return &instance;
}

void SSAOSystem::Initialize(UINT _width, UINT _height)
{
	CreateDescriptorsHeap();
	
	OnResize(_width, _height);
	CreateOffsetVectors();
	CreateRandomVectorTexture();
}

UINT SSAOSystem::Width() const
{
	return mRenderTargetWidth/2;
}

UINT SSAOSystem::Height() const
{
	return mRenderTargetHeight/2;
}

void SSAOSystem::GetOffsetVectors(XMFLOAT4 _offsets[14])
{
	std::copy(&mOffsets[0], &mOffsets[14], &_offsets[0]);
}

std::vector<float> SSAOSystem::CalcGaussWeights(float _sigma)
{
	float twoSigma2 = 2.0f*_sigma*_sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * _sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SSAOSystem::GetSrvDescriptorHeap() const
{
	return mSrvDescriptorHeap;
}

ID3D12Resource * SSAOSystem::NormalMap()
{
	return mNormalMap.Get();
}

ID3D12Resource * SSAOSystem::AmbientMap()
{
	return mAmbientMap0.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SSAOSystem::NormalMapRtv() const
{
	return mNormalMapCpuRtvHandle;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SSAOSystem::NormalMapSrv() const
{
	return mNormalMapGpuSrvHandle;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SSAOSystem::AmbientMapSrv() const
{
	return mAmbientMap0GpuSrvHandle;
}

void SSAOSystem::BindDescriptorsTable()
{
	// Descriptor Heap�� ���������ο� ���ε��Ѵ�.
	ID3D12DescriptorHeap* descriptorHeaps[] = { GetSrvDescriptorHeap().Get() };
	D3DApplication::Instance()->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Shader Resource View�� ���̴��� ���ε��Ѵ�.
	// �� �н������� �ش� Texture�� ���� ���� �ۼ��ϰ�
	// ���� �н����� �ش� �ؽ�ó�� ����, �׸��ڸ� ����Ͽ� ������ ��.
	D3DApplication::Instance()->CommandList()->SetGraphicsRootDescriptorTable((UINT)ROOT_SIGNATURE_ID::SSAO_MAP, AmbientMapSrv());
}

void SSAOSystem::CreateDescriptorsHeap()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	// ������ Heap�� �����ϱ� ����, Descriptor ����ü�� ����
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 5; // ���� ���� Shadow Map�� ������ �� �ִ�.
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// ������ Heap ����
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	// Screen Normal Map 1��, Ambient Map 2��
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvDescriptorHeap.GetAddressOf())));
}

void SSAOSystem::RegisterDescriptorsHandle(ID3D12Resource * depthStencilBuffer)
{
	// Descriptor�� ���� ������ �����Ѵ�.
	// SSAO�� 5���� ���� SRV�� ���� �� ������ Reserve�Ѵ�.

	UINT cbvSrvUavDescriptorSize = D3DApplication::Instance()->GetCbvSrvDescriptorSize();
	UINT rtvDescriptorSize = D3DApplication::Instance()->GetRtvDescriptorSize();
	
	mCpuSrvHandle = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mGpuSrvHandle = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	mCpuRtvHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	mAmbientMap0CpuSrvHandle = mCpuSrvHandle;
	mAmbientMap1CpuSrvHandle = mCpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mNormalMapCpuSrvHandle = mCpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mDepthMapCpuSrvHandle = mCpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mRandomVectorMapCpuSrvHandle= mCpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);

	mAmbientMap0GpuSrvHandle = mGpuSrvHandle;
	mAmbientMap1GpuSrvHandle = mGpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mNormalMapGpuSrvHandle = mGpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mDepthMapGpuSrvHandle = mGpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);
	mRandomVectorMapGpuSrvHandle = mGpuSrvHandle.Offset(1, cbvSrvUavDescriptorSize);

	mNormalMapCpuRtvHandle = mCpuRtvHandle;
	mAmbientMap0CpuRtvHandle = mCpuRtvHandle.Offset(1, rtvDescriptorSize);
	mAmbientMap1CpuRtvHandle = mCpuRtvHandle.Offset(1, rtvDescriptorSize);
	 
	CreateDescriptors(depthStencilBuffer);
}

void SSAOSystem::CreateDescriptors(ID3D12Resource * _depthStencilBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = NormalMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mNormalMapCpuSrvHandle);

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	device->CreateShaderResourceView(_depthStencilBuffer, &srvDesc, mDepthMapCpuSrvHandle);

	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	device->CreateShaderResourceView(mRandomVectorMap.Get(), &srvDesc, mRandomVectorMapCpuSrvHandle);

	srvDesc.Format = AmbientMapFormat;
	device->CreateShaderResourceView(mAmbientMap0.Get(), &srvDesc, mAmbientMap0CpuSrvHandle);
	device->CreateShaderResourceView(mAmbientMap1.Get(), &srvDesc, mAmbientMap1CpuSrvHandle);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = NormalMapFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mNormalMapCpuRtvHandle);

	rtvDesc.Format = AmbientMapFormat;
	device->CreateRenderTargetView(mAmbientMap0.Get(), &rtvDesc, mAmbientMap0CpuRtvHandle);
	device->CreateRenderTargetView(mAmbientMap1.Get(), &rtvDesc, mAmbientMap1CpuRtvHandle);
}

void SSAOSystem::SetPSOs(ID3D12PipelineState * _ssaoPso, ID3D12PipelineState * _ssaoBlurPso)
{
	mSsaoPso = _ssaoPso;
	mBlurPso = _ssaoBlurPso;
}

void SSAOSystem::OnResize(UINT _width, UINT _height)
{
	if (mRenderTargetWidth != _width || mRenderTargetHeight != _height)
	{
		mRenderTargetWidth = _width;
		mRenderTargetHeight = _height;

		// �ػ� ������ Ambient Map�� �������Ѵ�.
		mViewport.TopLeftX = 0.0f;
		mViewport.TopLeftY = 0.0f;
		mViewport.Width = mRenderTargetWidth / 2.0f;
		mViewport.Height = mRenderTargetHeight / 2.0f;
		mViewport.MinDepth = 0.0f;
		mViewport.MaxDepth = 1.0f;

		mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };

		CreateResources();
	}
}

void SSAOSystem::ComputeSSAO(int _blurCount, UploadBuffer<SSAOConstants> *_ssaoConstant)
{
	ID3D12GraphicsCommandList* commandList = D3DApplication::Instance()->CommandList().Get();
	
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);

	// �ʱ� �ֺ��� ���󵵸� AmbientMap0�� ����ؼ� �����Ѵ�.

	// ���� Ÿ���� �����Ѵ�.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(mAmbientMap0CpuRtvHandle, clearValue, 0, nullptr);

	// �������� ���۸� �����Ѵ�.
	commandList->OMSetRenderTargets(1, &mAmbientMap0CpuRtvHandle, true, nullptr);

	// ��� ���۸� ���ε��Ѵ�.
	auto ssaoCBAddress = _ssaoConstant->Resource()->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
	commandList->SetGraphicsRoot32BitConstant(1, 0, 0);
	
	// Rootsignature�� ����Ǿ���, �ٸ� ���� �ڿ�(Map)�� �����Ǿ����Ƿ� Heap�� ����ؾ��Ѵ�.
	ID3D12DescriptorHeap* descriptorHeaps[] = { GetSrvDescriptorHeap().Get() };
	D3DApplication::Instance()->CommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Normal Map�� Depth Map�� ���ε��Ѵ�.
	commandList->SetGraphicsRootDescriptorTable(2, mNormalMapGpuSrvHandle);

	// Random Vector Map�� ���ε��Ѵ�.
	commandList->SetGraphicsRootDescriptorTable(3, mRandomVectorMapGpuSrvHandle);

	// PSO�� �����Ѵ�.
	commandList->SetPipelineState(mSsaoPso);

	// ��ü ȭ�� ��鿡 �׸���.
	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->IASetIndexBuffer(nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);

	// �ڿ��� ���̴����� ���� �� �ֵ���, GENERIC_READ ���·� �ٲ۴�.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	BlurAmbientMap(commandList, _blurCount, _ssaoConstant);
}

void SSAOSystem::BlurAmbientMap(ID3D12GraphicsCommandList * _cmdList, int _blurCount, UploadBuffer<SSAOConstants> *_ssaoConstant)
{
	_cmdList->SetPipelineState(mBlurPso);

	auto ssaoCBAddress = _ssaoConstant->Resource()->GetGPUVirtualAddress();
	_cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

	for (int i = 0; i < _blurCount; ++i)
	{
		BlurAmbientMap(_cmdList, true);
		BlurAmbientMap(_cmdList, false);
	}
}

void SSAOSystem::BlurAmbientMap(ID3D12GraphicsCommandList * _cmdList, bool _horizontalBlur)
{
	ID3D12Resource* output = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

	// Ping-pong the two ambient map textures as we apply
	// horizontal and vertical blur passes.
	if (_horizontalBlur == true)
	{
		output = mAmbientMap1.Get();
		inputSrv = mAmbientMap0GpuSrvHandle;
		outputRtv = mAmbientMap1CpuRtvHandle;
		_cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
	}
	else
	{
		output = mAmbientMap0.Get();
		inputSrv = mAmbientMap1GpuSrvHandle;
		outputRtv = mAmbientMap0CpuRtvHandle;
		_cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
	}

	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	_cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);

	_cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

	// Normal/depth map still bound.

	// Normal Map�� Depth Map�� ���ε��Ѵ�.
	_cmdList->SetGraphicsRootDescriptorTable(2, mNormalMapGpuSrvHandle);

	// Ambient Map�� ���ε��Ѵ�.
	_cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);

	// ��ü ȭ�� ��鿡 �׸���.
	_cmdList->IASetVertexBuffers(0, 0, nullptr);
	_cmdList->IASetIndexBuffer(nullptr);
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->DrawInstanced(6, 1, 0, 0);

	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAOSystem::CreateResources()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	// Free the old resources if they exist.
	mNormalMap = nullptr;
	mAmbientMap0 = nullptr;
	mAmbientMap1 = nullptr;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mRenderTargetWidth;
	texDesc.Height = mRenderTargetHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = SSAOSystem::NormalMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	CD3DX12_CLEAR_VALUE optClear(NormalMapFormat, normalClearColor);
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(&mNormalMap)));

	// Ambient occlusion �ػ� ������ �����Ѵ�.
	texDesc.Width = mRenderTargetWidth / 2;
	texDesc.Height = mRenderTargetHeight / 2;
	texDesc.Format = SSAOSystem::AmbientMapFormat;

	float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	optClear = CD3DX12_CLEAR_VALUE(AmbientMapFormat, ambientClearColor);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mAmbientMap0)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mAmbientMap1)));
}

void SSAOSystem::CreateRandomVectorTexture()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mRandomVectorMap)));

	//
	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	//

	const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mRandomVectorMap.Get(), 0, num2DSubresources);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mRandomVectorMapUploadBuffer.GetAddressOf())));

	XMCOLOR initData[256 * 256];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			// Random vector in [0,1].  We will decompress in shader to [-1,1].
			XMFLOAT3 v(Math::RandF(), Math::RandF(), Math::RandF());

			initData[i * 256 + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	//
	// Schedule to copy the data to the default resource, and change states.
	// Note that mCurrSol is put in the GENERIC_READ state so it can be 
	// read by a shader.
	//

	ID3D12GraphicsCommandList* commandList = D3DApplication::Instance()->CommandList().Get();

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources(commandList, mRandomVectorMap.Get(), mRandomVectorMapUploadBuffer.Get(),
		0, 0, num2DSubresources, &subResourceData);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAOSystem::CreateOffsetVectors()
{
	// ���� ������ ���� 14���� ������ �д�.
	// �̵��� �Թ�ü�� �߽ɿ��� �Թ�ü�� ���� �������� �� ���� �߽� ���� ���� ���� ���͵��̴�.
	// �ݴ����� ���ϴ� �� ���͸� �迭 �ȿ��� ������ ���ҵ�� ��ġ�߱� ������,
	// 14�� �̸��� ǥ���� ����Ѵٰ��ص� �׻� ���� ������ ���͵��� ��� �ȴ�.

	// �Թ�ü ������ ���� ��
	mOffsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	mOffsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// �Թ�ü �� �߽� ���� ��
	mOffsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	mOffsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	mOffsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	mOffsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	mOffsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	mOffsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < 14; ++i)
	{
		//[0.25, 1.0] ������ ������ ���̸� ����Ѵ�.
		float s = Math::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));

		XMStoreFloat4(&mOffsets[i], v);
	}
}
