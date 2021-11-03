#ifndef _SSAO_SYSTEM_H_
#define _SSAO_SYSTEM_H_

class SSAOSystem final
{
private:

	SSAOSystem() = default;
	~SSAOSystem() = default;

	SSAOSystem(const SSAOSystem& _rhs) = delete;
	SSAOSystem& operator = (const SSAOSystem& _rhs) = delete;
	SSAOSystem(SSAOSystem&& _rhs) = delete;
	SSAOSystem& operator = (SSAOSystem&& _rhs) = delete;

public:

	static SSAOSystem* Instance();

public:

	void Initialize(UINT _width, UINT _height);

	void BindDescriptorsTable();
	void CreateDescriptorsHeap();
	void RegisterDescriptorsHandle(ID3D12Resource* depthStencilBuffer);
	void CreateDescriptors(ID3D12Resource* _depthStencilBuffer);

	void SetPSOs(ID3D12PipelineState* _ssaoPso, ID3D12PipelineState* _ssaoBlurPso);

	void OnResize(UINT newWidth, UINT newHeight);

	void ComputeSSAO(int _blurCount, UploadBuffer<SSAOConstants>* _ssaoConstant);

public:

	UINT Width() const;
	UINT Height() const;

	void GetOffsetVectors(XMFLOAT4 _offsets[14]);
	std::vector<float> CalcGaussWeights(float _sigma);
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() const;

	CD3DX12_GPU_DESCRIPTOR_HANDLE SRV() const;

	ID3D12Resource* NormalMap();
	ID3D12Resource* AmbientMap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv()const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv()const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE AmbientMapSrv()const;

private:

	void BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, int _blurCount, UploadBuffer<SSAOConstants>* _ssaoConstant);
	void BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, bool _horizontalBlur);
	void CreateResources();
	void CreateRandomVectorTexture();
	void CreateOffsetVectors();

public:

	static const int MaxBlurRadius = 5;

	static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
	static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

private:

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ID3D12PipelineState* mSsaoPso = nullptr;
	ID3D12PipelineState* mBlurPso = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNormalMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientMap0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientMap1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mNormalMapCpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNormalMapGpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mNormalMapCpuRtvHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mDepthMapCpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mDepthMapGpuSrvHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mRandomVectorMapCpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mRandomVectorMapGpuSrvHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mAmbientMap0CpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mAmbientMap0GpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mAmbientMap0CpuRtvHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mAmbientMap1CpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mAmbientMap1GpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mAmbientMap1CpuRtvHandle;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuRtvHandle;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	DirectX::XMFLOAT4 mOffsets[14];
};

#endif