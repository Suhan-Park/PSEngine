#include "CapsuleCollider.h"

CapsuleCollider::CapsuleCollider()
{
	SetCapsuleSize(1.0f, 1.0f);
}

CapsuleCollider::CapsuleCollider(float _radius, float _height)
{
	SetCapsuleSize(_radius, _height);
}

void CapsuleCollider::SetCapsuleSize(float _radius, float _height)
{
	mBoundingBoundary.x = _radius;
	mBoundingBoundary.y = _height;
}

void CapsuleCollider::SetUpGeometry()
{
	// Half Height를 인자로 받음.
	mPxGeometry = new PxCapsuleGeometry(mBoundingBoundary.x, mBoundingBoundary.y / 2.0f);
}