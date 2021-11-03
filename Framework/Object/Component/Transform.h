#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "Component.h"

class Transform final : public Component
{
public:

	Transform();
	virtual ~Transform();

private:

	Transform(const Transform& _rhs) = delete;
	Transform& operator = (const Transform& _rhs) = delete;
	Transform(Transform&& _rhs) = delete;
	Transform& operator = (Transform&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime)override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime)override;
	virtual void Destroy() override;

public:

	inline const XMFLOAT3 GetWorldPosition() const
	{ 
		return mPosition;
	}
	inline const XMFLOAT3 GetWorldRotation() const
	{
		return mRotation;
	}
	inline const XMFLOAT3 GetWorldScale() const
	{ 
		return mScale;
	}

	inline const XMFLOAT3 GetLocalPosition() const
	{ 
		return mLocalPosition;
	}
	inline const XMFLOAT3 GetLocalRotation() const
	{
		return mLocalRotation;
	}
	inline const XMFLOAT3 GetLocalScale() const
	{ 
		return mLocalScale;
	}

	inline XMVECTOR GetRightVector()
	{
		return XMVECTOR(RotationMatrix().r[0]);
	}
	inline XMVECTOR GetUpVector()
	{
		return XMVECTOR(RotationMatrix().r[1]);
	}
	inline XMVECTOR GetForwardVector()
	{
		return XMVECTOR(RotationMatrix().r[2]);
	}
	inline const XMMATRIX& WorldMatrix() const
	{
		return XMLoadFloat4x4(&mTransformMatrix);
	}
	inline const XMMATRIX& ScaleMatrix() const
	{
		return XMLoadFloat4x4(&mWorldScaleMatrix);
	}
	inline const XMMATRIX& RotationMatrix() const
	{
		return XMLoadFloat4x4(&mWorldRotationMatrix);
	}
	inline const XMMATRIX& TranslationMatrix() const
	{
		return XMLoadFloat4x4(&mWorldTranslationMatrix);
	}

public:

	void SetWorldPosition(XMFLOAT3 _position);
	void SetWorldPosition(float _x, float _y, float _z);
	void SetWorldRotation(XMFLOAT3 _eulerAngles);
	void SetWorldRotation(float _x, float _y, float _z);
	void SetWorldScale(XMFLOAT3 _scale);
	void SetWorldScale(float _x, float _y, float _z);
	void SetLocalPosition(XMFLOAT3 _position);
	void SetLocalPosition(float _x, float _y, float _z);
	void SetLocalRotation(XMFLOAT3 _eulerAngles);
	void SetLocalRotation(float _x, float _y, float _z);
	void SetLocalScale(XMFLOAT3 _scale);
	void SetLocalScale(float _x, float _y, float _z);
	void Translate(XMFLOAT3 _translation, Transform* _relativeObject);
	void Translate(XMFLOAT3 _translation, bool _relativeToLocal = false);
	void Rotate(XMFLOAT3 _eulerAngles, bool _relativeToLocal = false);
	void Rotate(XMFLOAT3 _axis, FLOAT _angle, bool _relativeToLocal = false);
	void RotateAround(XMFLOAT3 _point, XMFLOAT3 _axis, FLOAT _angle);
	void LookAt(XMFLOAT3 _position);
	void SetParent(Transform* _parent);

private:

	void UpdateWorldTransformMatrix();
	void UpdateWorldScaleMatrix();
	void UpdateWorldRotationMatrix();
	void UpdateWorldTranslationMatrix();
	void UpdateLocalScaleMatrix();
	void UpdateLocalRotationMatrix();
	void UpdateLocalTranslationMatrix();
	void UpdateChild();

private:

	Transform* mParent = nullptr;
	std::list<Transform*> mChilds;

	XMFLOAT3 mPosition = XMFLOAT3(0.0f,0.0f,0.0f);
	XMFLOAT3 mScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMFLOAT3 mRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMFLOAT3 mLocalPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mLocalScale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMFLOAT3 mLocalRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMFLOAT3 mForwardVector = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mRightVector = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 mUpVector = XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMFLOAT4X4 mTransformMatrix = Math::Identity4x4();
	XMFLOAT4X4 mWorldScaleMatrix = Math::Identity4x4();
	XMFLOAT4X4 mWorldRotationMatrix = Math::Identity4x4();
	XMFLOAT4X4 mWorldTranslationMatrix = Math::Identity4x4();

	XMFLOAT4X4 mLocalScaleMatrix = Math::Identity4x4();
	XMFLOAT4X4 mLocalRotationMatrix = Math::Identity4x4();
	XMFLOAT4X4 mLocalTranslationMatrix = Math::Identity4x4();
};

#endif