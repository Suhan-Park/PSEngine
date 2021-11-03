#include "CameraController.h"

#include "Input.h"
#include "Transform.h"

void CameraController::Awake()
{
}

void CameraController::Update(const float _deltaTime)
{
	if (Input::GetKey(KEYCODE::W))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 0.0f, -1.0f * _deltaTime));
	}

	if (Input::GetKey(KEYCODE::A))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(1.0f * _deltaTime, 0.0f, 0.0f));
	}

	if (Input::GetKey(KEYCODE::S))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 0.0f, 1.0f * _deltaTime));
	}

	if (Input::GetKey(KEYCODE::D))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(-1.0f * _deltaTime, 0.0f, 0.0f));
	}

	if (Input::GetKey(KEYCODE::Q))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 1.0f * _deltaTime, 0.0f));
	}

	if (Input::GetKey(KEYCODE::E))
	{
		GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, -1.0f * _deltaTime, 0.0f));
	}
}

void CameraController::FixedUpdate(const float _fixedDltaTime)
{
}

void CameraController::Draw(const float _deltaTime)
{

}

void CameraController::Destroy()
{
}
