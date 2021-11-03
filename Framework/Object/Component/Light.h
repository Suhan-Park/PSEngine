#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "Component.h"

enum class LightType : char {Directional, Point, Spot};

class Light : public Component
{
public:
	
	Light() = default;
	virtual ~Light() = default;

private:

	Light(const Light& _rhs) = delete;
	Light& operator = (const Light&& _rhs) = delete;
	Light(Light&& _rhs) = delete;
	Light& operator = (Light&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime)final;
	virtual void FixedUpdate(const float _fixedDltaTime) final;
	virtual void Draw(const float _deltaTime)final;
	virtual void Destroy() override;

public:

	const LightType GetType() const;
	const XMFLOAT4 GetDiffuse() const;
	const FLOAT GetRange() const;
	const FLOAT GetIntensity() const;
	const FLOAT GetSpotAngle() const;
	const FLOAT NearZ() const;
	const FLOAT FarZ() const;

	const XMFLOAT3 LightPosition() const;
	const XMMATRIX ViewMatrix() const;
	const XMMATRIX ProjectionMatrix() const;
	const XMMATRIX ShadowTransformMatrix() const;

	void SetType(const LightType _type);
	void SetDiffuse(const XMFLOAT4 _diffuse);
	void SetAmbient(const XMFLOAT4 _ambient);
	void SetSpecular(const XMFLOAT4 _specular);
	void SetRange(const FLOAT _range);
	void SetIntensity(const FLOAT _intensity);
	void SetSpotAngle(const FLOAT _angle);


private:
	
	class Transform* mTransform = nullptr;

	LightType mType = LightType::Directional;
	XMFLOAT4 mDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 mAmbient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 mSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	DirectX::BoundingSphere mBoundSphere;

	FLOAT mNearZ = 0.0f;
	FLOAT mFarZ = 0.0f;
	FLOAT mRange = 1.0f;
	FLOAT mIntensity = 1.0f; 
	FLOAT mSpotAngle = 30.0f;

	XMFLOAT3 mLightPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT4X4 mLightViewMatrix = Math::Identity4x4();
	XMFLOAT4X4 mLightProjectionMatrix = Math::Identity4x4();
	XMFLOAT4X4 mShadowTransformMatrix = Math::Identity4x4();
};

#endif // !_LIGHT_H_
