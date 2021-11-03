#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "D3DApplication.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

#include "ResourceUploadBatch.h"

class Texture
{
public:

	Texture(std::string _name, std::wstring _filePath) : mName(_name), mFilePath(_filePath) { }
	Texture() {}
	~Texture() = default;

private:

	Texture(const Texture& _rhs) = delete;
	Texture& operator = (const Texture& _rhs) = delete;
	Texture(Texture&& _rhs) = delete;
	Texture& operator = (Texture&& _rhs) = delete;

public:

	inline void Initialize(bool _isDDS = true)
	{
		if (_isDDS)
		{
			CreateTextureFromDDSFileFormat();
		}
		else
		{
			CreateTextureFromWICFileFormat();
		}
	}

	inline void CreateTextureFromDDSFileFormat()
	{
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
			D3DApplication::Instance()->Device().Get(),
			D3DApplication::Instance()->CommandList().Get(),
			mFilePath.c_str(),
			mResource,
			mUploadHeap
		));
	}

	inline void CreateTextureFromWICFileFormat()
	{
		ResourceUploadBatch upload(D3DApplication::Instance()->Device().Get());
		upload.Begin();

		ID3D12Resource* resource = nullptr;
		DirectX::CreateWICTextureFromFile(
			D3DApplication::Instance()->Device().Get(),
			upload,
			mFilePath.c_str(),
			&resource
		);

		mResource = resource;

		// 리소스를 GPU로 업로드한다.
		auto finish = upload.End(D3DApplication::Instance()->CommandQueue().Get());

		// 업로드 스레드가 종료될 때 까지 기다린다.
		finish.wait();
	}

	inline void CreateTexureFromPData(void* _pcData, UINT _rowPitch, UINT _slicePitch, UINT64 _width, UINT64 _height)
	{
		D3D12_RESOURCE_DESC  desc;
		ZeroMemory(&desc, sizeof(D3D12_RESOURCE_DESC));

		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = 800;
		textureDesc.Height = 600;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// 기본힙에 텍스처를 생성한다.
		ThrowIfFailed(D3DApplication::Instance()->Device()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&mResource)));

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mResource.Get(), 0, 1);

		// 업로드힙에 텍스처를 생성한다.
		ThrowIfFailed(D3DApplication::Instance()->Device()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadHeap)));

		D3D12_SUBRESOURCE_DATA subResource = {};
		subResource.pData = _pcData;

		UpdateSubresources(D3DApplication::Instance()->CommandList().Get(), mResource.Get(), mUploadHeap.Get(), 0, 0, 1, &subResource);
		D3DApplication::Instance()->CommandList()->ResourceBarrier(
			1, 
			&CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	inline ID3D12Resource* Get()
	{
		if (mResource)
		{
			return mResource.Get();
		}

		return nullptr;
	}

	inline const std::string Name() const
	{
		return mName;
	}

	inline const std::wstring File() const
	{
		return mFilePath;
	}

private:

	std::string mName;
	std::wstring mFilePath;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};

#endif