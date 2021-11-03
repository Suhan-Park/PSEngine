#pragma once

#include "Component.h"

#ifndef _BOX_H_
#define _BOX_H_

class BoxController : public Component
{
public:

	BoxController() = default;
	virtual ~BoxController() = default;

private:

	BoxController(const BoxController& _rhs) = delete;
	BoxController& operator = (const BoxController& _rhs) = delete;
	BoxController(BoxController&& _rhs) = delete;
	BoxController& operator = (BoxController&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;
};

#endif // !_BOX_H_
