#include "BoxCollider.h"
#include "PhysXSystem.h"

BoxCollider::BoxCollider()
{
	SetBoxSize(XMFLOAT3(1.0f,1.0f,1.0f));
}

BoxCollider::BoxCollider(XMFLOAT3 _boundingBoxSize)
{
	SetBoxSize(_boundingBoxSize);
}

void BoxCollider::SetBoxSize(XMFLOAT3 _boundingBoxSize)
{
	mBoundingBoundary = _boundingBoxSize;
}

void BoxCollider::SetUpGeometry()
{
	mPxGeometry = new PxBoxGeometry(mBoundingBoundary.x * 0.5f, mBoundingBoundary.y * 0.5f, mBoundingBoundary.z * 0.5f);
}