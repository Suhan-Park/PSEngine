#ifndef _BOX_COLLIDER_H_
#define _BOX_COLLIDER_H_

#include "Collider.h"

class BoxCollider final : public Collider
{
public:

	BoxCollider();
	BoxCollider(XMFLOAT3 _boundingBoxSize);
	virtual ~BoxCollider() = default;

private:

	BoxCollider(const BoxCollider& _rhs) = delete;
	BoxCollider& operator = (const BoxCollider& _rhs) = delete;
	BoxCollider(BoxCollider&& _rhs) = delete;
	BoxCollider& operator = (BoxCollider&& _rhs) = delete;

public:

	void SetBoxSize(XMFLOAT3 _boundingBoxSize);

protected:

	virtual void SetUpGeometry() override;
};

#endif // !_BOX_COLLIDER_H_
