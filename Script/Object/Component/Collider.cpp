#include "Collider.h"
#include "PhysXSystem.h"

#include "RigidBody.h"

void Collider::Awake()
{
	mRigidBody = GetComponent<RigidBody>();
	mMaterial = PhysXSystem::Instance()->GetPhysics()->createMaterial(mStaticFriction, mDynamicFriction, mRestitution);

	SetUpPxShape();
	mRigidBody->AttachPxShape(mShape);
}

void Collider::Update(const float _deltaTime)
{
}

void Collider::FixedUpdate(const float _fixedDltaTime)
{
}

void Collider::Draw(const float _deltaTime)
{
}

void Collider::Destroy()
{
	mRigidBody->DettachPxShape(mShape);
}

void Collider::SetPhyXMaterial(float _staticFriction, float _dynamicFriction, float _restitution)
{
	mStaticFriction = _staticFriction;
	mDynamicFriction = _dynamicFriction;
	mRestitution = _restitution;

	if (nullptr == mMaterial)
	{
		mMaterial->release();
	}

	mMaterial = PhysXSystem::Instance()->GetPhysics()->createMaterial(mStaticFriction, mDynamicFriction, mRestitution);
	SetUpPxShape();
}

void Collider::SetOffset(XMFLOAT3 _offset)
{
	mOffset = _offset;
	mPxTransform = PxTransform(mOffset.x, mOffset.y, mOffset.z);
	 
	SetUpPxShape();
}

void Collider::SetTrigger(bool _isTrigger)
{
	mTrigger = _isTrigger;

	SetUpPxShape();
}

void Collider::SetUpPxShape()
{
	if (nullptr != mShape)
	{
		mShape->release();
	}

	if (nullptr == mPxGeometry)
	{
		delete mPxGeometry;
	}

	SetUpGeometry();

	mShape = PhysXSystem::Instance()->GetPhysics()->createShape(*mPxGeometry, *mMaterial);
	mShape->setLocalPose(mPxTransform);
	mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mTrigger);
	mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mTrigger);
}
