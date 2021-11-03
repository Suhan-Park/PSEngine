#include "PlaneCollider.h"

PlaneCollider::PlaneCollider()
{
}

void PlaneCollider::SetUpGeometry()
{
	mPxGeometry = new PxPlaneGeometry();
}
