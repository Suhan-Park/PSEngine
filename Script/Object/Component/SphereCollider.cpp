#include "SphereCollider.h"

SphereCollider::SphereCollider()
{
	SetSphereRadius(1.0f);
}

SphereCollider::SphereCollider(float _boundingSphereRadius)
{
	SetSphereRadius(_boundingSphereRadius);
}

void SphereCollider::SetSphereRadius(float _boundingSphereRadius)
{
	mBoundingBoundary.x = _boundingSphereRadius;
	mBoundingBoundary.y = _boundingSphereRadius;
	mBoundingBoundary.z = _boundingSphereRadius;
}

void SphereCollider::SetUpGeometry()
{
	mPxGeometry = new PxSphereGeometry(mBoundingBoundary.x);
}
