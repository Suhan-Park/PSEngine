#ifndef _COLLIDER_H_
#define _COLLIDER_H_

#include "Component.h"

class RigidBody;

using namespace physx;

class Collider abstract : public Component
{
public:

	Collider() = default;
	virtual ~Collider() = default;

private:

	Collider(const Collider& _rhs) = delete;
	Collider& operator = (const Collider& _rhs) = delete;
	Collider(Collider&& _rhs) = delete;
	Collider& operator = (Collider&& _rhs) = delete;

public:

	virtual void Awake() final;
	virtual void Update(const float _deltaTime) final;
	virtual void FixedUpdate(const float _fixedDltaTime) final;
	virtual void Draw(const float _deltaTime) final;
	virtual void Destroy() final;

public:

	inline XMFLOAT3 GetOffset()
	{
		return mOffset;
	}

	inline bool GetIsTrigger()
	{
		return mTrigger;
	}

	void SetPhyXMaterial(float _staticFriction, float _dynamicFriction, float _restitution);
	void SetOffset(XMFLOAT3 _offset);
	void SetTrigger(bool _isTrigger);

protected:

	virtual void SetUpGeometry() = 0;

private:

	void SetUpPxShape();

protected:

	PxShape*    mShape = nullptr;
	PxMaterial* mMaterial = nullptr;
	PxGeometry* mPxGeometry = nullptr;
	RigidBody*  mRigidBody = nullptr;
	PxTransform mPxTransform = PxTransform(0.0f, 0.0f, 0.0f);

	// PROPERTY
	XMFLOAT3    mBoundingBoundary;
	bool        mTrigger = false;
	float       mStaticFriction = 0.5f;
	float       mDynamicFriction = 0.5f;
	float       mRestitution = 0.5f;

private:

	XMFLOAT3 mOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
};

#endif // !_COLLIDER_H
