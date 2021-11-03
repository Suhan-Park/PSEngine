#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "Component.h"
#include "Math.h"

class Camera : public Component
{
public:

	Camera(float _width, float _height);
	virtual ~Camera();

private:

	Camera(const Camera& _rhs) = delete;
	Camera& operator =(const Camera& _rhs) = delete;
	Camera(Camera&& _rhs) = delete;
	Camera& operator =(Camera&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	static Camera* Main()
	{
		if (nullptr == mMainCamera)
		{
			return nullptr;
		}

		return mMainCamera;
	}

	const DirectX::XMMATRIX ViewMatrix();
	const DirectX::XMMATRIX ProjectionMatrix();
	const DirectX::XMMATRIX ViewProjectionMatrix();

	const float FOV() const { return mFieldOfView; }
	const float NearZ() const { return mNearZ; }
	const float FarZ() const { return mFarZ; }

	// 창 크기를 변경할 때, 호출된다.
	void ResizeWindow(float _width, float _height);

private:

	static Camera* mMainCamera;

	class Transform* mTransform;

	XMFLOAT4X4 mWorld = Math::Identity4x4();
	XMFLOAT4X4 mView = Math::Identity4x4();
	XMFLOAT4X4 mProjection = Math::Identity4x4();
	
	XMVECTOR mPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR mTarget = XMVectorZero();
	XMVECTOR mUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	D3D12_VIEWPORT mViewPort;

	float mAspectRatio = 0.0f;
	float mFieldOfView = 45.0f;
	float mNearZ = 1.0f;
	float mFarZ = 1000.0f;

};
#endif //_CAMERA_H_