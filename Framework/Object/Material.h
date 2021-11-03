#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Object.h"
#include "Texture.h"

enum class RENDERING_MODE
{
	Opaque = 0x00,
	CutOut,
	Fade,
	TransParent
};

class Material : public Object
{
public:
	
	Material() = default;
	Material(std::string _name);

	virtual ~Material() = default;

private:

	Material(const Material& _rhs) = delete;
	Material& operator = (const Material& _rhs) = delete;
	Material(Material&& _rhs) = delete;
	Material& operator = (Material&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;

	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	inline XMFLOAT4 GetAlbedo()
	{
		return mAlbedo;
	}

	inline XMFLOAT4 GetAmbient()
	{
		return mAmbient;
	}

	inline XMFLOAT4 GetSpecular()
	{
		return mSpecular;
	}

	inline void SetAlbedo(XMFLOAT4 _albedo)
	{
		mAlbedo = _albedo;
	}

	inline void SetAmbient(XMFLOAT4 _ambient)
	{
		mAmbient = _ambient;
	}

	inline void SetSpecular(XMFLOAT4 _specular)
	{
		mSpecular = _specular;
	}

	inline void SetOffset(XMFLOAT2 _offset)
	{
		mOffset = _offset;
	}

	inline void SetTiling(XMFLOAT2 _tiling)
	{
		mTiling = _tiling;
	}

	inline void SetFresnelR0(XMFLOAT3 _fresnelR0)
	{
		mFresnelR0 = _fresnelR0;
	}	

	inline void SetMetalic(FLOAT _degree)
	{
		mMetallic = _degree;
	}

	inline void SetShineness(FLOAT _degree)
	{
		mShineness = _degree;
	}

	inline const Texture* DiffuseMap() const
	{
		return mDiffuseMap;
	}

	inline const Texture* NormalMap() const
	{
		return mNormalMap;
	}

	inline const Texture* SpecularMap() const
	{
		return mSpecularMap;
	}

public:

	void AttachDiffuseMap(Texture* _texture);
	void AttachNormalMap(Texture* _texture);
	void AttachSpecularMap(Texture* _texture);

private:
	
	void CreateShaderResourceHeapAndView();
	void UpdateMaterialBuffer();

private:

	Texture* mDiffuseMap = nullptr;
	Texture* mNormalMap = nullptr;
	Texture* mSpecularMap = nullptr;
	
	FLOAT mMetallic = FLOAT(1.0f);
	FLOAT mShineness = FLOAT(1.0f);
	
	XMFLOAT2 mTiling = XMFLOAT2(1.0f, 1.0f);
	XMFLOAT2 mOffset = XMFLOAT2(1.0f, 1.0f);

	XMFLOAT3 mFresnelR0 = XMFLOAT3(1.0f, 1.0f, 1.0f);

	XMFLOAT4 mAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 mAmbient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 mSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	MaterialData mMaterialData;

	UINT mDiffuseMapFlag = 0;
	UINT mNormalMapFlag = 0;
	UINT mSpecularMapFlag = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialBuffer = nullptr;

	UINT mMatBufferIndex = 0;

private:

	void _AWAKE_() final;
	void _UPDATE_(const float _deltaTime) final;
	void _DRAW_(const float _deltaTime) final;
	void _DESTORY_() final;
};

#endif // _MATERIAL_H_
