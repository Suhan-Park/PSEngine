#include "BoxController.h"

#include "Transform.h"

void BoxController::Awake()
{
}

void BoxController::Update(const float _deltaTime)
{
	GetComponent<Transform>()->Rotate(XMFLOAT3(0.0f, -90.0f * _deltaTime, 0.0f));
}

void BoxController::FixedUpdate(const float _fixedDltaTime)
{
}

void BoxController::Draw(const float _deltaTime)
{

}

void BoxController::Destroy()
{
}
