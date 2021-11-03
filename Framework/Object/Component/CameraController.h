#pragma once

#include "Component.h"

#ifndef _CAMERA_CONTROLLER_H_
#define _CAMERA_CONTROLLER_H_

class CameraController : public Component
{
public:

	CameraController() = default;
	virtual ~CameraController() = default;

private:

	CameraController(const CameraController& _rhs) = delete;
	CameraController& operator = (const CameraController& _rhs) = delete;
	CameraController(CameraController&& _rhs) = delete;
	CameraController& operator = (CameraController&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;
};

#endif // !_CAMERA_CONTROLLER_H_
