#include "Camera.h"

#include "Transform.h"

Camera* Camera::mMainCamera = nullptr;

Camera::Camera(float _width, float _height)
{
	ResizeWindow(_width, _height);
}

Camera::~Camera()
{

}

const DirectX::XMMATRIX Camera::ViewMatrix()
{
	XMMATRIX rotation = mTransform->RotationMatrix();
	XMMATRIX translation = mTransform->TranslationMatrix();

	return  XMMatrixMultiply(rotation, translation);
}

const DirectX::XMMATRIX Camera::ProjectionMatrix()
{
	XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(mFieldOfView, mAspectRatio, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProjection, proj);

	return proj;
}

const DirectX::XMMATRIX Camera::ViewProjectionMatrix()
{
	XMMATRIX view = ViewMatrix();
	XMMATRIX proj = ProjectionMatrix();

	return view * proj;
}

void Camera::ResizeWindow(float _width, float _height)
{
	mAspectRatio = _width / _height;
}

void Camera::Awake()
{
	if (nullptr == mMainCamera)
	{
		mMainCamera = this;
	}

	mTransform = GetComponent<Transform>();
}

void Camera::Update(const float _deltaTime)
{

}

void Camera::FixedUpdate(const float _fixedDltaTime)
{
}

void Camera::Draw(const float _deltaTime)
{

}

void Camera::Destroy()
{
	mMainCamera = nullptr;
}