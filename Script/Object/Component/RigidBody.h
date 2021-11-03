#ifndef _RIGID_BODY_H
#define _RIGID_BODY_H

#include "Component.h"

using namespace physx;

class Transform;

enum class RIGIDBODY_TYPE : char
{
	STATIC, DYNAMIC
};

class RigidBody final : public Component
{
public:

	RigidBody() = default;
	virtual ~RigidBody() = default;

private:

	RigidBody(const RigidBody& _rhs) = delete;
	RigidBody& operator = (const RigidBody& _rhs) = delete;
	RigidBody(RigidBody&& _rhs) = delete;
	RigidBody& operator = (RigidBody&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	void AttachPxShape(PxShape* _shape);
	void DettachPxShape(PxShape* _shape);

	float    GetMass();
	float    GetLinearDamping();
	float    GetAngularDamping();
	bool     GetKinematic();
	bool     GetGravity();
	XMFLOAT3 GetMassPose();
	XMFLOAT3 GetVelocity();
	XMFLOAT3 GetAngularVelocity();

	void SetRigidBodyType(RIGIDBODY_TYPE _type);
	void SetMass(float _mass);
	void SetLinearDamping(float _drag);
	void SetAngularDamping(float _drag);
	void SetFreeze(bool _posX, bool _posY, bool _posZ, bool _rotX, bool _rotY, bool _rotZ);
	void SetKinematic(bool _isActive);
	void SetGravity(bool _isActive);
	void SetMassPose(XMFLOAT3 _position);
	void SetVelocity(XMVECTOR _velocity);
	void SetAngularVelocity(XMVECTOR _velocity);
	void AddTorque(XMVECTOR _torque);
	void AddForce(XMVECTOR _force);

private:
	
	Transform*      mTransform = nullptr;
	PxRigidStatic*  mRigidStatic = nullptr;
	PxRigidDynamic* mRigidDynamic = nullptr;
	RIGIDBODY_TYPE  mRigidBodyType;
	
	XMFLOAT3    mPosition;
	XMFLOAT3    mRotation;
	XMVECTOR    mQuaternion;
	PxQuat      mPxQuaternion;
	PxTransform mPxTransform;
};

#endif // !_RIGID_BODY_H
