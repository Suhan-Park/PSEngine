#ifndef _PLANE_COLLIDER_H_
#define _PLANE_COLLIDER_H_

#include "Collider.h"

class PlaneCollider final : public Collider
{
public:

	PlaneCollider();
	virtual ~PlaneCollider() = default;

private:

	PlaneCollider(const PlaneCollider& _rhs) = delete;
	PlaneCollider& operator = (const PlaneCollider& _rhs) = delete;
	PlaneCollider(PlaneCollider&& _rhs) = delete;
	PlaneCollider& operator = (PlaneCollider&& _rhs) = delete;

protected:

	virtual void SetUpGeometry() override;
};

#endif // !_PLANE_COLLIDER_H_
