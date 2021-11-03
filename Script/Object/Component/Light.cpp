#include "Light.h"
#include "Transform.h"
#include "LightSystem.h"

void Light::Awake()
{
	LightSystem::Instance()->AddLight(this, mType);
	mTransform = GetComponent<Transform>();

	mBoundSphere.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mBoundSphere.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

void Light::Update(const float _deltaTime)
{
	// �켱�� ����-���� ������ ����Ѵ�. (Point, Spot Light�� ���Ŀ� ����)
	if (LightType::Directional == mType)
	{
		// Directional Light�̾, Depth ������ ���ؼ� Position�� ������ �־�� �Ѵ�.
		// ����, �׸��� ������ ���� View ���, Projection ����� �ʿ��ϴ�.
		XMVECTOR lightDir = mTransform->GetForwardVector();
		XMVECTOR lightPos = -2.0f*mBoundSphere.Radius*lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&mBoundSphere.Center);
		XMVECTOR lightUp = mTransform->GetUpVector();
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&mLightPosition, lightPos);
		
		// Shadow Map�� ��ҿ� ���ؼ� ����� �� �ʿ� �����Ƿ�, ��� ������ �����Ѵ�
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, ViewMatrix()));

		// Direction Light�� ���� �������� Shadow Map�� �����Ѵ�. (�Ÿ��� ������ �ȹް� �׸��� ���)
		float l = sphereCenterLS.x - mBoundSphere.Radius;
		float b = sphereCenterLS.y - mBoundSphere.Radius;
		float n = sphereCenterLS.z - mBoundSphere.Radius;
		float r = sphereCenterLS.x + mBoundSphere.Radius;
		float t = sphereCenterLS.y + mBoundSphere.Radius;
		float f = sphereCenterLS.z + mBoundSphere.Radius;

		mNearZ = n;
		mFarZ = f;
		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
		
		// NDC �������� [-1,+1]->R^2 �ؽ�ó �������� [0,1]->R^2 ��ȯ�ϱ� ���� ���
		const XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		// ī�޶󿡼� �������� ��ȯ�ϱ� ���� ����� �ʿ��ϴ�.
		// ���� �� ����� ����, Shadow Map�� Depth ����, ����-������ �Ÿ� ������ �Ͽ� �׸��ڸ� �帮���.
		XMMATRIX S = lightView * lightProj * T;
		XMStoreFloat4x4(&mLightViewMatrix, lightView);
		XMStoreFloat4x4(&mLightProjectionMatrix, lightProj);
		XMStoreFloat4x4(&mShadowTransformMatrix, S);
	}
}

void Light::FixedUpdate(const float _fixedDltaTime)
{
}

void Light::Draw(const float _deltaTime)
{
}

void Light::Destroy()
{
}

const LightType Light::GetType() const
{
	return mType;
}

const XMFLOAT4 Light::GetDiffuse() const
{
	return mDiffuse;
}

const FLOAT Light::GetRange() const
{
	return mRange;
}

const FLOAT Light::GetIntensity() const
{
	return mIntensity;
}

const FLOAT Light::GetSpotAngle() const
{
	return mSpotAngle;
}

const FLOAT Light::NearZ() const
{
	return mNearZ;
}

const FLOAT Light::FarZ() const
{
	return mFarZ;
}

const XMFLOAT3 Light::LightPosition() const
{
	return mLightPosition;
}

const XMMATRIX Light::ViewMatrix() const
{
	return XMLoadFloat4x4(&mLightViewMatrix);
}

const XMMATRIX Light::ProjectionMatrix() const
{
	return XMLoadFloat4x4(&mLightProjectionMatrix);
}

const XMMATRIX Light::ShadowTransformMatrix() const
{
	return XMLoadFloat4x4(&mShadowTransformMatrix);
}

void Light::SetType(const LightType _type)
{
	LightSystem::Instance()->RemoveLight(this, mType);

	mType = _type;
	LightSystem::Instance()->AddLight(this, mType);
}

void Light::SetDiffuse(const XMFLOAT4 _diffuse)
{
	mDiffuse = _diffuse;
}

void Light::SetAmbient(const XMFLOAT4 _ambient)
{
	mAmbient = _ambient;
}

void Light::SetSpecular(const XMFLOAT4 _specular)
{
	mSpecular = _specular;
}

void Light::SetRange(const FLOAT _range)
{
	mRange = _range;
}

void Light::SetIntensity(const FLOAT _intensity)
{
	mIntensity = _intensity;
}

void Light::SetSpotAngle(const FLOAT _angle)
{
	mSpotAngle = _angle;
}
