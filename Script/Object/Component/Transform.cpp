#include "Transform.h"

Transform::Transform()
{
}

Transform::~Transform()
{
	mParent = nullptr;
	mChilds.clear();
}

void Transform::Awake()
{
}

// ������Ʈ���� ����� �����ϰԵǸ�, ���� ������ ���� ���� Transform Ȥ�� �ٸ� ������Ʈ���� ������� �� �����Ƿ� ������ ������ �� �� �ִ�.
void Transform::Update(const float _deltaTime)
{
}

void Transform::FixedUpdate(const float _fixedDltaTime)
{
}

void Transform::Draw(const float _deltaTime)
{
}

void Transform::Destroy()
{
}

void Transform::SetWorldPosition(XMFLOAT3 _position)
{
	mPosition = _position;

	XMMATRIX worldTranslationMatrix = XMMatrixIdentity();
	worldTranslationMatrix.r[3].m128_f32[0] = _position.x;
	worldTranslationMatrix.r[3].m128_f32[1] = _position.y;
	worldTranslationMatrix.r[3].m128_f32[2] = _position.z;
	worldTranslationMatrix.r[3].m128_f32[3] = 1.0f;

	XMStoreFloat4x4(&mWorldTranslationMatrix, worldTranslationMatrix);

	// Local Position�� Local Translation Matrix ����
	UpdateLocalTranslationMatrix();

	// World Matrix ����
	UpdateWorldTransformMatrix();

	UpdateChild();
}

void Transform::SetWorldPosition(float _x, float _y, float _z)
{
	SetWorldPosition(XMFLOAT3(_x, _y, _z));
}

void Transform::SetWorldRotation(XMFLOAT3 _eulerAngles)
{
	mRotation = _eulerAngles;

	// ȸ���� Roll-Pitch-Yaw (Z-X-Y) ����
	FLOAT pitch = XMConvertToRadians(_eulerAngles.x);
	FLOAT yaw = XMConvertToRadians(_eulerAngles.y);
	FLOAT roll = XMConvertToRadians(_eulerAngles.z);

	XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat);
	XMStoreFloat4x4(&mWorldRotationMatrix, rotationMatrix);

	// Local Rotation�� Local Rotation Matrix ����
	UpdateLocalRotationMatrix();
	// World Matrix ����
	UpdateWorldTransformMatrix();

	UpdateChild();
}

void Transform::SetWorldRotation(float _x, float _y, float _z)
{
	SetWorldRotation(XMFLOAT3(_x, _y, _z));
}

void Transform::SetWorldScale(XMFLOAT3 _scale)
{
	mScale = _scale;

	XMMATRIX worldScaleMatrix = XMMatrixScaling(_scale.x, _scale.y, _scale.z);
	XMStoreFloat4x4(&mWorldScaleMatrix, worldScaleMatrix);

	// Local Scale�� Local Scale Matrix ����
	UpdateLocalScaleMatrix();
	// World Matrix ����
	UpdateWorldTransformMatrix();
	
	UpdateChild();
}

void Transform::SetWorldScale(float _x, float _y, float _z)
{
	SetWorldScale(XMFLOAT3(_x, _y, _z));
}

void Transform::SetLocalPosition(XMFLOAT3 _position)
{
	XMMATRIX localTranslationMatrix = XMMatrixIdentity();
	localTranslationMatrix.r[3].m128_f32[0] = _position.x;
	localTranslationMatrix.r[3].m128_f32[1] = _position.y;
	localTranslationMatrix.r[3].m128_f32[2] = _position.z;
	localTranslationMatrix.r[3].m128_f32[3] = 1.0f;

	XMStoreFloat4x4(&mLocalTranslationMatrix, localTranslationMatrix);

	// World Position�� World Translation Matrix ����
	UpdateWorldTranslationMatrix();
	// World Matrix ����
	UpdateWorldTransformMatrix();

	UpdateChild();
}

void Transform::SetLocalPosition(float _x, float _y, float _z)
{
	SetLocalPosition(XMFLOAT3(_x, _y, _z));
}

void Transform::SetLocalRotation(XMFLOAT3 _eulerAngles)
{
	mLocalRotation = _eulerAngles;

	// ȸ���� Roll-Pitch-Yaw (Z-X-Y) ����
	FLOAT pitch = XMConvertToRadians(_eulerAngles.x);
	FLOAT yaw = XMConvertToRadians(_eulerAngles.y);
	FLOAT roll = XMConvertToRadians(_eulerAngles.z);

	XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat);
	XMStoreFloat4x4(&mLocalRotationMatrix, rotationMatrix);

	// World Rotation�� World Rotation Matrix ����
	UpdateWorldRotationMatrix();
	// World Matrix ����
	UpdateWorldTransformMatrix();

	UpdateChild();
}

void Transform::SetLocalRotation(float _x, float _y, float _z)
{
	SetLocalRotation(XMFLOAT3(_x, _y, _z));
}

void Transform::SetLocalScale(XMFLOAT3 _scale)
{
	mLocalScale = _scale;

	XMMATRIX localScaleMatrix = XMMatrixScaling(_scale.x, _scale.y, _scale.z);
	XMStoreFloat4x4(&mLocalScaleMatrix, localScaleMatrix);

	// World Scale�� World Scale Matrix ����
	UpdateLocalScaleMatrix();
	// World Matrix ����
	UpdateWorldTransformMatrix();

	UpdateChild();
}

void Transform::SetLocalScale(float _x, float _y, float _z)
{
	SetLocalScale(XMFLOAT3(_x, _y, _z));
}

void Transform::Translate(XMFLOAT3 _vector3, bool _relativeToLocal)
{
	if (_relativeToLocal)
	{
		// ������Ʈ�� ���� ��ǥ�� �������� �̵� -> ������Ʈ ȸ�� ����� �̿��Ͽ� ������ ȸ�� �� �̵�
		XMMATRIX translationMatrix = XMMatrixTranslation(_vector3.x, _vector3.y, _vector3.z);
		XMMATRIX translationMatrixLocalCoordinate = XMLoadFloat4x4(&mWorldRotationMatrix) * translationMatrix;

		SetWorldPosition(XMFLOAT3(mPosition.x + translationMatrixLocalCoordinate.r[3].m128_f32[0],
								  mPosition.y + translationMatrixLocalCoordinate.r[3].m128_f32[1], 
								  mPosition.z + translationMatrixLocalCoordinate.r[3].m128_f32[2]));
	}
	else
	{
		// ���� ��ǥ�� �������� �̵� -> ���� ��ǥ�� �������� ���� �� ��ŭ �̵�
		SetWorldPosition(XMFLOAT3(mPosition.x + _vector3.x, mPosition.y + _vector3.y, mPosition.z + _vector3.z));
	}
}

void Transform::Translate(XMFLOAT3 _vector3, Transform * _relativeObject)
{
	if(nullptr == _relativeObject)
	{
		return;
	}

	// ������Ʈ�� ���� ��ǥ�� �������� �̵� -> ������Ʈ ȸ�� ����� �̿��Ͽ� ������ ȸ�� �� �̵�
	XMMATRIX translationMatrix = XMMatrixTranslation(_vector3.x, _vector3.y, _vector3.z);
	XMMATRIX translationMatrixLocalCoordinate = XMLoadFloat4x4(&_relativeObject->mWorldRotationMatrix) * translationMatrix;

	SetWorldPosition(XMFLOAT3(mPosition.x + translationMatrixLocalCoordinate.r[3].m128_f32[0],
							  mPosition.y + translationMatrixLocalCoordinate.r[3].m128_f32[1],
							  mPosition.z + translationMatrixLocalCoordinate.r[3].m128_f32[2]));
}

void Transform::Rotate(XMFLOAT3 _eulerAngles, bool _relativeToLocal)
{
	if (_relativeToLocal)
	{
		FLOAT roll = XMConvertToRadians(_eulerAngles.z);
		FLOAT pitch = XMConvertToRadians(_eulerAngles.x);
		FLOAT yaw = XMConvertToRadians(_eulerAngles.y);

		XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);

		// Local ȸ���̹Ƿ� ���� Local ��ǥ�踦 �������� ��ȯ�� �����ϰ�, World ȸ���� �����Ѵ�.
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat) * XMLoadFloat4x4(&mWorldRotationMatrix);
		
		XMFLOAT3 rotation = XMFLOAT3();
		Math::DecomposeRollPitchYawFromMatrix(rotationMatrix, rotation.x, rotation.y, rotation.z);

		SetWorldRotation(rotation);
	}
	else
	{
		SetWorldRotation(XMFLOAT3(mRotation.x + _eulerAngles.x, mRotation.y + _eulerAngles.y, mRotation.z + _eulerAngles.z));
	}
}

void Transform::Rotate(XMFLOAT3 _axis, FLOAT _angle, bool _relativeToLocal)
{
	XMVECTOR axis = XMVectorZero();
	axis.m128_f32[0] = _axis.x;
	axis.m128_f32[1] = _axis.y;
	axis.m128_f32[2] = _axis.z;
	axis.m128_f32[3] = 0.0f;

	XMMATRIX rotater = XMMatrixRotationAxis(axis, _angle);
	XMMATRIX rotationMatrix = XMMatrixIdentity();

	if (_relativeToLocal)
	{
		// Local ȸ���̹Ƿ� ���� Local ��ǥ�踦 �������� ��ȯ�� �����ϰ�, World ȸ���� �����Ѵ�.
		rotationMatrix = rotater * XMLoadFloat4x4(&mWorldRotationMatrix);
	}
	else
	{
		// ���� ��ǥ�� �������� ȸ��
		rotationMatrix = XMLoadFloat4x4(&mWorldRotationMatrix) * rotater;
	}

	XMFLOAT3 rotation = XMFLOAT3();
	Math::DecomposeRollPitchYawFromMatrix(rotationMatrix, rotation.x, rotation.y, rotation.z);

	SetWorldRotation(rotation);
}

void Transform::LookAt(XMFLOAT3 _position)
{
	XMVECTOR from;
	from.m128_f32[0] = mPosition.x;
	from.m128_f32[1] = mPosition.y;
	from.m128_f32[2] = mPosition.z;
	from.m128_f32[3] = 1.0f;

	XMVECTOR to;
	to.m128_f32[0] = _position.x;
	to.m128_f32[1] = _position.y;
	to.m128_f32[2] = _position.z;
	to.m128_f32[3] = 1.0f;
	
	XMVECTOR forward = to - from;
	XMVECTOR right = XMVector3Cross(forward, XMVECTOR({ 0.0f,1.0f,0.0f,0.0f }));
	XMVECTOR up = XMVector3Cross(right, forward);

	XMMATRIX rotationMatrix = XMMatrixIdentity();
	rotationMatrix.r[0].m128_f32[0] = right.m128_f32[0];
	rotationMatrix.r[0].m128_f32[1] = right.m128_f32[1];
	rotationMatrix.r[0].m128_f32[2] = right.m128_f32[2];
	rotationMatrix.r[0].m128_f32[3] = right.m128_f32[3];

	rotationMatrix.r[1].m128_f32[0] = up.m128_f32[0];
	rotationMatrix.r[1].m128_f32[1] = up.m128_f32[1];
	rotationMatrix.r[1].m128_f32[2] = up.m128_f32[2];
	rotationMatrix.r[1].m128_f32[3] = up.m128_f32[3];

	rotationMatrix.r[2].m128_f32[0] = forward.m128_f32[0];
	rotationMatrix.r[2].m128_f32[1] = forward.m128_f32[1];
	rotationMatrix.r[2].m128_f32[2] = forward.m128_f32[2];
	rotationMatrix.r[2].m128_f32[3] = forward.m128_f32[3];	
	
	XMFLOAT3 rotation = XMFLOAT3();
	Math::DecomposeRollPitchYawFromMatrix(rotationMatrix, rotation.x, rotation.y, rotation.z);

	SetWorldRotation(rotation);
}

void Transform::RotateAround(XMFLOAT3 _point, XMFLOAT3 _axis, FLOAT _angle)
{
	XMVECTOR axis = XMVectorZero();
	axis.m128_f32[0] = _axis.x;
	axis.m128_f32[1] = _axis.y;
	axis.m128_f32[2] = _axis.z;
	axis.m128_f32[3] = 0.0f;

	XMMATRIX rotater = XMMatrixRotationAxis(axis, _angle);
	XMMATRIX translater = XMMatrixTranslation(mPosition.x - _point.x, mPosition.y - _point.y, mPosition.z - _point.z);
	XMMATRIX inverseTranslater = XMMatrixTranslation(_point.x, _point.y, _point.z);
	
	XMMATRIX TransformMatrix = translater * rotater * inverseTranslater;

	XMFLOAT3 rotation = XMFLOAT3();
	Math::DecomposeRollPitchYawFromMatrix(rotater, rotation.x, rotation.y, rotation.z);

	rotation.x += mRotation.x;
	rotation.y += mRotation.y;
	rotation.z += mRotation.z;

	SetWorldRotation(rotation);
	SetWorldPosition(XMFLOAT3(TransformMatrix.r[3].m128_f32[0], TransformMatrix.r[3].m128_f32[1], TransformMatrix.r[3].m128_f32[2]));
}

void Transform::SetParent(Transform* const _parent)
{
	// �ڱ� �ڽ��� �θ�� �Ѱ��� ���� ��,
	if (this == _parent)
	{
		return;
	}

	// ���� �θ��� ��,
	if (mParent == _parent)
	{
		return;
	}

	// Parent Transform�� nullptr�� �Ѱ��־��� ��.
	if (nullptr == _parent)
	{
		// �θ𿡼� �ڱ� �ڽ��� �����
		// Remove �Լ��� ��ġ�ϴ� ��� ��Ҹ� �����Ѵ�. ������ ��Ҵ� ������ �������.
		// �����δ� ������ ��Ҹ� �������� ����� ������ ������ ��ġ���� ������ ������ ��ȿȭ��.
		// �̷��� ������ ä���� ���� ������ erase�� �̿��ؼ� ��������Ѵ�.
		// �Ʒ� �Լ��� std::remove�� ����� ���Ƿ� ä���� ������ �ݺ��ڰ� ��ȯ�� �������� �����̳��� �� ��������(end()) erase�Լ��� �̿��ؼ� �����ִ� ������ �Ѵ�.
		// mComponents.erase(std::remove(mComponents.begin(), mComponents.end(), _component), mComponents.end());

		mParent->mChilds.erase(std::remove(mParent->mChilds.begin(), mParent->mChilds.end(), this), mParent->mChilds.end());
		mParent = nullptr;

		// Update Local Scale, Rotation, Translation Matrix
		UpdateLocalScaleMatrix();
		UpdateLocalRotationMatrix();
		UpdateLocalTranslationMatrix();

		return;
	}
	
	// �θ� ���� ��, �ٸ� �θ� Ʈ���������� ������ ���.
	if (mParent)
	{
		// ���� �θ𿡼� �ڽ��� �����.
		mParent->mChilds.erase(std::remove(mParent->mChilds.begin(), mParent->mChilds.end(), this), mParent->mChilds.end());
	}

	// �θ� ���� �� + �θ� ������ ��� (�ʱ����)

	// ���ο� �θ� �����ϰ� �߰��Ѵ�.
	mParent = _parent;
	mParent->mChilds.emplace_back(this);

	// Update Local Scale, Rotation, Translation Matrix
	UpdateLocalScaleMatrix();
	UpdateLocalRotationMatrix();
	UpdateLocalTranslationMatrix();
}

// World Transform Matrix ���� (Local�� �״��)
void Transform::UpdateWorldTransformMatrix()
{
	XMMATRIX worldMatrix = XMMatrixIdentity();

	if (mParent)
	{
		worldMatrix = XMLoadFloat4x4(&mLocalScaleMatrix)
			* XMLoadFloat4x4(&mLocalRotationMatrix)
			* XMLoadFloat4x4(&mLocalTranslationMatrix)
			* XMLoadFloat4x4(&mParent->mTransformMatrix);

		XMVECTOR scale;
		XMVECTOR rotQuat;
		XMVECTOR translation;

		XMMatrixDecompose(&scale, &rotQuat, &translation, worldMatrix);

		XMStoreFloat3(&mPosition, translation);

		// Quaternion To Euler Angles -> Convert Degree
		XMFLOAT4 eluer;
		XMStoreFloat4(&eluer, rotQuat);
		
		XMFLOAT3 rot = Math::QuaternionToEuler(eluer);
		mRotation.x = XMConvertToDegrees(rot.x);
		mRotation.y = XMConvertToDegrees(rot.y);
		mRotation.z = XMConvertToDegrees(rot.z);

		XMStoreFloat3(&mScale, scale);

		XMStoreFloat4x4(&mWorldScaleMatrix, XMMatrixScaling(mScale.x, mScale.y, mScale.z));
		XMStoreFloat4x4(&mWorldRotationMatrix, XMMatrixRotationRollPitchYaw(eluer.x, eluer.y, eluer.z));
		XMStoreFloat4x4(&mWorldTranslationMatrix, XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z));
	}
	else
	{
		worldMatrix = XMLoadFloat4x4(&mWorldScaleMatrix) * XMLoadFloat4x4(&mWorldRotationMatrix) * XMLoadFloat4x4(&mWorldTranslationMatrix);
	}

	XMStoreFloat4x4(&mTransformMatrix, worldMatrix);
}

// �θ� �̵��ϰų�, Local Scale�� ����� ��, ȣ��Ǵ� �Լ�.
void Transform::UpdateWorldScaleMatrix()
{
	XMMATRIX localScaleMatrix = XMLoadFloat4x4(&mLocalScaleMatrix);

	if (mParent)
	{
		XMMATRIX parentScaleMatrix = XMLoadFloat4x4(&mParent->mWorldScaleMatrix);
		XMMATRIX worldScaleMatrix = parentScaleMatrix * localScaleMatrix;

		mScale.x = worldScaleMatrix.r[0].m128_f32[0];
		mScale.y = worldScaleMatrix.r[1].m128_f32[1];
		mScale.z = worldScaleMatrix.r[2].m128_f32[2];

		XMStoreFloat4x4(&mWorldScaleMatrix, worldScaleMatrix);
	}
	else
	{
		// Parent Transform�� ���� ���, Local Scale�� World Scale�� ����.
		mScale = mLocalScale;
		XMStoreFloat4x4(&mWorldScaleMatrix, localScaleMatrix);
	}
}

//  �θ� �̵��ϰų�, Local Rotation�� ����� ��, ȣ��Ǵ� �Լ�.
void Transform::UpdateWorldRotationMatrix()
{
	XMMATRIX localRotationMatrix = XMLoadFloat4x4(&mLocalRotationMatrix);

	if (mParent)
	{
		XMMATRIX parentWorldRotationMatrix = XMLoadFloat4x4(&mParent->mWorldRotationMatrix);
		XMMATRIX worldRotationMatrix = parentWorldRotationMatrix * localRotationMatrix;

		XMFLOAT3 rotation = XMFLOAT3();
		Math::DecomposeRollPitchYawFromMatrix(worldRotationMatrix, rotation.x, rotation.y, rotation.z);

		mRotation.x = XMConvertToDegrees(rotation.x);
		mRotation.y = XMConvertToDegrees(rotation.y);
		mRotation.z = XMConvertToDegrees(rotation.z);
		XMStoreFloat4x4(&mWorldRotationMatrix, worldRotationMatrix);
	}
	else
	{
		// Parent Transform�� ���� ���, Local Rotatiom�� World Rotation�� ����.
		mRotation = mLocalRotation;
		XMStoreFloat4x4(&mWorldRotationMatrix, localRotationMatrix);
	}
}

//  �θ� �̵��ϰų�, Local Position�� ����� ��, ȣ��Ǵ� �Լ�.
void Transform::UpdateWorldTranslationMatrix()
{
	XMMATRIX localTranslationMatrix = XMLoadFloat4x4(&mLocalTranslationMatrix);

	if (mParent)
	{
		XMMATRIX parentTranslationMatrix = XMLoadFloat4x4(&mParent->mWorldTranslationMatrix);
		XMMATRIX worldTranslationMatrix = parentTranslationMatrix * localTranslationMatrix;

		mPosition.x = worldTranslationMatrix.r[3].m128_f32[0];
		mPosition.y = worldTranslationMatrix.r[3].m128_f32[1];
		mPosition.z = worldTranslationMatrix.r[3].m128_f32[2];

		XMStoreFloat4x4(&mWorldTranslationMatrix, worldTranslationMatrix);
	}
	else
	{
		// Parent Transform�� ���� ���, Local Position�� World Position �� ����.
		mPosition = mLocalPosition;
		XMStoreFloat4x4(&mWorldTranslationMatrix, localTranslationMatrix);
	}
}

// World Scale�� ����� ��, ȣ��Ǵ� �Լ�. (Ȥ�� Parent Transform�� ����� ��)
void Transform::UpdateLocalScaleMatrix()
{
	XMMATRIX worldScaleMatrix = XMLoadFloat4x4(&mWorldRotationMatrix);

	if (mParent)
	{
		XMMATRIX parentInverseScaleMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mParent->mWorldScaleMatrix));
		XMMATRIX localScaleMatrix = worldScaleMatrix * parentInverseScaleMatrix;

		mLocalScale.x = localScaleMatrix.r[0].m128_f32[0];
		mLocalScale.y = localScaleMatrix.r[1].m128_f32[1];
		mLocalScale.z = localScaleMatrix.r[2].m128_f32[2];

		XMStoreFloat4x4(&mLocalScaleMatrix, localScaleMatrix);
	}
	else
	{
		// Parent Transform�� ���� ���, Local Scale�� World Scale�� ����.
		mLocalScale = mScale;
		XMStoreFloat4x4(&mLocalScaleMatrix, worldScaleMatrix);
	}
}

// World Rotation�� ����� ��, ȣ��Ǵ� �Լ�. (Ȥ�� Parent Transform�� ����� ��)
void Transform::UpdateLocalRotationMatrix()
{
	XMMATRIX worldRotationMatrix = XMLoadFloat4x4(&mWorldRotationMatrix);
	
	if (mParent)
	{
		XMMATRIX parentInverseRotationMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mParent->mWorldRotationMatrix));
		XMMATRIX localRotationMatrix = worldRotationMatrix * parentInverseRotationMatrix;

		XMFLOAT3 rotation = XMFLOAT3();
		Math::DecomposeRollPitchYawFromMatrix(localRotationMatrix, rotation.x, rotation.y, rotation.z);

		// Parent Transform�� ���� ���, Local Rotaion�� World Rotation�� ����.
		mLocalRotation.x = XMConvertToDegrees(rotation.x);
		mLocalRotation.y = XMConvertToDegrees(rotation.y);
		mLocalRotation.z = XMConvertToDegrees(rotation.z);
		XMStoreFloat4x4(&mLocalRotationMatrix, localRotationMatrix);
	}
	else
	{
		mLocalRotation = mRotation;
		XMStoreFloat4x4(&mLocalRotationMatrix, worldRotationMatrix);
	}
}

// World Position�� ����� ��, ȣ��Ǵ� �Լ�. (Ȥ�� Parent Transform�� ����� ��)
void Transform::UpdateLocalTranslationMatrix()
{
	XMMATRIX worldTranslationMatrix = XMLoadFloat4x4(&mWorldTranslationMatrix);

	if (mParent)
	{
		XMMATRIX parentInverseTranslationMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mParent->mWorldTranslationMatrix));
		XMMATRIX localTranslationMatrix = worldTranslationMatrix * parentInverseTranslationMatrix;

		// Parent Transform�� ���� ���, Local Position�� World Position�� ����.
		mLocalPosition.x = localTranslationMatrix.r[3].m128_f32[0];
		mLocalPosition.y = localTranslationMatrix.r[3].m128_f32[1];
		mLocalPosition.z = localTranslationMatrix.r[3].m128_f32[2];

		XMStoreFloat4x4(&mLocalTranslationMatrix, localTranslationMatrix);
	}
	else
	{
		mLocalPosition = mPosition;
		XMStoreFloat4x4(&mLocalTranslationMatrix, worldTranslationMatrix);
	}

}

void Transform::UpdateChild()
{
	// �θ��� Transform Matrix �����ͼ�, Child�� Local Matrix �̿��ؼ� World Matrix�� ����.
	// World Position, Rotation, Scale ���� �ٲ� �� ����
	// Grand Child�� ����
	// ����, Child�� �����ϱ� ���ؼ� ��İ��� �ؾ��ϰ� Decompose�� �ؾ��ϴµ�, �̰� �� ������ ������?
	if (!mChilds.empty())
	{
		for (auto child : mChilds)
		{
			child->UpdateWorldScaleMatrix();
			child->UpdateWorldRotationMatrix();
			child->UpdateWorldTranslationMatrix();

			child->UpdateWorldTransformMatrix();
		}
	}
}
