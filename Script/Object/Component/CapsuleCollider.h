#ifndef _CAPSULE_COLLIDER_H_
#define _CAPSULE_COLLIDER_H_

#include "Collider.h"

class CapsuleCollider final : public Collider
{
public:

	CapsuleCollider();
	CapsuleCollider(float _radius, float _height);
	virtual ~CapsuleCollider() = default;

private:

	CapsuleCollider(const CapsuleCollider& _rhs) = delete;
	CapsuleCollider& operator = (const CapsuleCollider& _rhs) = delete;
	CapsuleCollider(CapsuleCollider&& _rhs) = delete;
	CapsuleCollider& operator = (CapsuleCollider&& _rhs) = delete;

public:

	void SetCapsuleSize(float _radius, float _height);

protected:

	virtual void SetUpGeometry() override;
};

#endif // !_CAPSULE_COLLIDER_H_
