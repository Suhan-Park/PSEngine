#include "RigidBody.h"

#include "PhysXSystem.h"
#include "Transform.h"

void RigidBody::Awake()
{
	mTransform = GetComponent<Transform>();
	mPosition = mTransform->GetWorldPosition();
	mRotation = mTransform->GetWorldRotation();
	mQuaternion = XMQuaternionRotationRollPitchYaw(mRotation.x / RADIAN, mRotation.y / RADIAN, mRotation.z / RADIAN);

	mPxQuaternion.x = mQuaternion.m128_f32[0];
	mPxQuaternion.y = mQuaternion.m128_f32[1];
	mPxQuaternion.z = mQuaternion.m128_f32[2];
	mPxQuaternion.w = mQuaternion.m128_f32[3];

	mPxTransform = PxTransform(mPosition.x, mPosition.y, mPosition.z, mPxQuaternion);

	if (RIGIDBODY_TYPE::STATIC == mRigidBodyType)
	{
		mRigidStatic = PhysXSystem::Instance()->GetPhysics()->createRigidStatic(mPxTransform);
		mRigidStatic->userData = GetOwner();

		PhysXSystem::Instance()->GetScene()->addActor(*mRigidStatic);
	}
	else if (RIGIDBODY_TYPE::DYNAMIC == mRigidBodyType)
	{
		mRigidDynamic = PhysXSystem::Instance()->GetPhysics()->createRigidDynamic(mPxTransform);
		mRigidDynamic->userData = GetOwner();

		SetMass(10.0f);
		SetGravity(true);
		SetKinematic(false);
		SetLinearDamping(0.5f);
		SetAngularDamping(0.5f);
		SetFreeze(false, false, false, false, false, false);

		PhysXSystem::Instance()->GetScene()->addActor(*mRigidDynamic);
	}
}

void RigidBody::Update(const float _deltaTime)
{
	if (RIGIDBODY_TYPE::DYNAMIC != mRigidBodyType)
	{
		return;
	}

	// 스크립트에서 업데이트 된 Position 값을 갱신한다.
	mPosition = mTransform->GetWorldPosition();
	mPxTransform.p = PxVec3(mPosition.x, mPosition.y, mPosition.z);
	
	// 스크립트에서 업데이트 된 Rotation 값을 갱신한다.
	mRotation = mTransform->GetWorldRotation();
	mQuaternion = XMQuaternionRotationRollPitchYaw(mRotation.x / RADIAN, mRotation.y / RADIAN, mRotation.z / RADIAN);

	mPxQuaternion.x = mQuaternion.m128_f32[0];
	mPxQuaternion.y = mQuaternion.m128_f32[1];
	mPxQuaternion.z = mQuaternion.m128_f32[2];
	mPxQuaternion.w = mQuaternion.m128_f32[3];

	if (mPxQuaternion.isSane())
	{
		mPxTransform.q = mPxQuaternion;
		mRigidDynamic->setGlobalPose(mPxTransform);
	}
}

void RigidBody::FixedUpdate(const float _fixedDltaTime)
{
	if (RIGIDBODY_TYPE::DYNAMIC != mRigidBodyType)
	{
		return;
	}

	// Phys X Scene에서 업데이트 된 Position 값을 반영한다.
	mPosition.x = mRigidDynamic->getGlobalPose().p.x;
	mPosition.y = mRigidDynamic->getGlobalPose().p.y;
	mPosition.z = mRigidDynamic->getGlobalPose().p.z;
	mTransform->SetWorldPosition(mPosition);

	// Phys X Scene에서 업데이트 된 Rotation 값을 반영한다.
	mPxQuaternion = mRigidDynamic->getGlobalPose().q;
	mQuaternion.m128_f32[0] = mPxQuaternion.x;
	mQuaternion.m128_f32[1] = mPxQuaternion.y;
	mQuaternion.m128_f32[2] = mPxQuaternion.z;
	mQuaternion.m128_f32[3] = mPxQuaternion.w;
	XMMATRIX rotMatrix = XMMatrixRotationQuaternion(mQuaternion);

	float rotX = 0.0f;
	float rotY = 0.0f;
	float rotZ = 0.0f;
	Math::DecomposeRollPitchYawFromMatrix(rotMatrix, rotX, rotY, rotZ);

	mQuaternion = XMQuaternionRotationRollPitchYaw(rotX, rotY, rotZ);
	mPxQuaternion.x = mQuaternion.m128_f32[0];
	mPxQuaternion.y = mQuaternion.m128_f32[1];
	mPxQuaternion.z = mQuaternion.m128_f32[2];
	mPxQuaternion.w = mQuaternion.m128_f32[3];

	if (mPxQuaternion.isSane())
	{
		rotX *= RADIAN;
		rotY *= RADIAN;
		rotZ *= RADIAN;
		mTransform->SetWorldRotation(rotX, rotY, rotZ);
	}
}

void RigidBody::Draw(const float _deltaTime)
{

}

void RigidBody::Destroy()
{
	if (mRigidBodyType == RIGIDBODY_TYPE::STATIC)
	{
		PhysXSystem::Instance()->GetScene()->removeActor(*mRigidStatic);
		mRigidStatic->release();
		mRigidStatic = nullptr;
	}
	else if (mRigidBodyType == RIGIDBODY_TYPE::DYNAMIC)
	{
		PhysXSystem::Instance()->GetScene()->removeActor(*mRigidDynamic);
		mRigidDynamic->release();
		mRigidDynamic = nullptr;
	}
}

void RigidBody::AttachPxShape(PxShape * _shape)
{
	physx::PxFilterData filterData;
	_shape->setSimulationFilterData(filterData);

	if (mRigidBodyType == RIGIDBODY_TYPE::STATIC)
	{
		mRigidStatic->attachShape(*_shape);
	}
	else if (mRigidBodyType == RIGIDBODY_TYPE::DYNAMIC)
	{
		mRigidDynamic->attachShape(*_shape);
	}
}

void RigidBody::DettachPxShape(PxShape * _shape)
{
	if (mRigidBodyType == RIGIDBODY_TYPE::STATIC)
	{
		mRigidStatic->detachShape(*_shape);
	}
	else if (mRigidBodyType == RIGIDBODY_TYPE::DYNAMIC)
	{
		mRigidDynamic->detachShape(*_shape);
	}
}

float RigidBody::GetMass()
{
	if (nullptr == mRigidDynamic)
	{
		return 0.0f;
	}

	return mRigidDynamic->getMass();
}

float RigidBody::GetLinearDamping()
{
	if (nullptr == mRigidDynamic)
	{
		return 0.0f;
	}

	return mRigidDynamic->getLinearDamping();
}

float RigidBody::GetAngularDamping()
{
	if (nullptr == mRigidDynamic)
	{
		return 0.0f;
	}

	return mRigidDynamic->getAngularDamping();
}

bool RigidBody::GetKinematic()
{
	if (nullptr == mRigidDynamic)
	{
		return false;
	}

	return mRigidDynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC);
}

bool RigidBody::GetGravity()
{
	if (nullptr == mRigidDynamic)
	{
		return false;
	}

	return mRigidDynamic->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_GRAVITY);
}

XMFLOAT3 RigidBody::GetMassPose()
{
	if (nullptr == mRigidDynamic)
	{
		return XMFLOAT3(0.0f,0.0f,0.0f);
	}

	return XMFLOAT3(mRigidDynamic->getCMassLocalPose().p.x, mRigidDynamic->getCMassLocalPose().p.y, mRigidDynamic->getCMassLocalPose().p.z);
}

XMFLOAT3 RigidBody::GetVelocity()
{
	if (nullptr == mRigidDynamic)
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	return XMFLOAT3(mRigidDynamic->getLinearVelocity().x, mRigidDynamic->getLinearVelocity().y, mRigidDynamic->getLinearVelocity().z);
}

XMFLOAT3 RigidBody::GetAngularVelocity()
{
	if (nullptr == mRigidDynamic)
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	return XMFLOAT3(mRigidDynamic->getAngularVelocity().x, mRigidDynamic->getAngularVelocity().y, mRigidDynamic->getAngularVelocity().z);
}

void RigidBody::SetRigidBodyType(RIGIDBODY_TYPE _type)
{
	mRigidBodyType = _type;
}

void RigidBody::SetMass(float _mass)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setMass(_mass);
}

void RigidBody::SetLinearDamping(float _drag)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setLinearDamping(_drag);
}

void RigidBody::SetAngularDamping(float _drag)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setAngularDamping(_drag);
}

void RigidBody::SetFreeze(bool _posX, bool _posY, bool _posZ, bool _rotX, bool _rotY, bool _rotZ)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	PxRigidDynamicLockFlags flags;

	if (_posX)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
	}

	if (_posY)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
	}

	if (_posZ)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
	}

	if (_rotX)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
	}

	if (_rotY)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
	}

	if (_rotZ)
	{
		flags |= physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
	}

	mRigidDynamic->setRigidDynamicLockFlags(flags);
}

void RigidBody::SetKinematic(bool _isActive)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, _isActive);
}

void RigidBody::SetGravity(bool _isActive)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !_isActive);
}

void RigidBody::SetMassPose(XMFLOAT3 _position)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setCMassLocalPose(PxTransform(PxVec3(_position.x, _position.y, _position.z)));
}

void RigidBody::SetVelocity(XMVECTOR _velocity)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setLinearVelocity(PxVec3(_velocity.m128_f32[0], _velocity.m128_f32[1], _velocity.m128_f32[2]));
}

void RigidBody::SetAngularVelocity(XMVECTOR _velocity)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->setAngularVelocity(PxVec3(_velocity.m128_f32[0], _velocity.m128_f32[1], _velocity.m128_f32[2]));
}

void RigidBody::AddTorque(XMVECTOR _torque)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->addTorque(PxVec3(_torque.m128_f32[0], _torque.m128_f32[1], _torque.m128_f32[2]));
}

void RigidBody::AddForce(XMVECTOR _force)
{
	if (nullptr == mRigidDynamic)
	{
		return;
	}

	mRigidDynamic->addForce(PxVec3(_force.m128_f32[0], _force.m128_f32[1], _force.m128_f32[2]));
}