#ifndef _SPHERE_COLLIDER_H_
#define _SPHERE_COLLIDER_H_

#include "Collider.h"

class SphereCollider final : public Collider
{
public:

	SphereCollider();
	SphereCollider(float _boundingSphereRadius);
	virtual ~SphereCollider() = default;

private:

	SphereCollider(const SphereCollider& _rhs) = delete;
	SphereCollider& operator = (const SphereCollider& _rhs) = delete;
	SphereCollider(SphereCollider&& _rhs) = delete;
	SphereCollider& operator = (SphereCollider&& _rhs) = delete;

public:

	void SetSphereRadius(float _boundingSphereRadius);

protected:

	virtual void SetUpGeometry() override;
};

#endif // !_SPHERE_COLLIDER_H_
