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

// 업데이트에서 행렬을 갱신하게되면, 실행 순서에 따라 하위 Transform 혹은 다른 컴포넌트들이 영향받을 수 있으므로 버그의 원인이 될 수 있다.
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

	// Local Position과 Local Translation Matrix 갱신
	UpdateLocalTranslationMatrix();

	// World Matrix 갱신
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

	// 회전은 Roll-Pitch-Yaw (Z-X-Y) 순서
	FLOAT pitch = XMConvertToRadians(_eulerAngles.x);
	FLOAT yaw = XMConvertToRadians(_eulerAngles.y);
	FLOAT roll = XMConvertToRadians(_eulerAngles.z);

	XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat);
	XMStoreFloat4x4(&mWorldRotationMatrix, rotationMatrix);

	// Local Rotation과 Local Rotation Matrix 갱신
	UpdateLocalRotationMatrix();
	// World Matrix 갱신
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

	// Local Scale과 Local Scale Matrix 갱신
	UpdateLocalScaleMatrix();
	// World Matrix 갱신
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

	// World Position과 World Translation Matrix 갱신
	UpdateWorldTranslationMatrix();
	// World Matrix 갱신
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

	// 회전은 Roll-Pitch-Yaw (Z-X-Y) 순서
	FLOAT pitch = XMConvertToRadians(_eulerAngles.x);
	FLOAT yaw = XMConvertToRadians(_eulerAngles.y);
	FLOAT roll = XMConvertToRadians(_eulerAngles.z);

	XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quat);
	XMStoreFloat4x4(&mLocalRotationMatrix, rotationMatrix);

	// World Rotation과 World Rotation Matrix 갱신
	UpdateWorldRotationMatrix();
	// World Matrix 갱신
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

	// World Scale과 World Scale Matrix 갱신
	UpdateLocalScaleMatrix();
	// World Matrix 갱신
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
		// 오브젝트의 로컬 좌표계 기준으로 이동 -> 오브젝트 회전 행렬을 이용하여 기저를 회전 후 이동
		XMMATRIX translationMatrix = XMMatrixTranslation(_vector3.x, _vector3.y, _vector3.z);
		XMMATRIX translationMatrixLocalCoordinate = XMLoadFloat4x4(&mWorldRotationMatrix) * translationMatrix;

		SetWorldPosition(XMFLOAT3(mPosition.x + translationMatrixLocalCoordinate.r[3].m128_f32[0],
								  mPosition.y + translationMatrixLocalCoordinate.r[3].m128_f32[1], 
								  mPosition.z + translationMatrixLocalCoordinate.r[3].m128_f32[2]));
	}
	else
	{
		// 월드 좌표계 기준으로 이동 -> 월드 좌표계 기준으로 벡터 값 만큼 이동
		SetWorldPosition(XMFLOAT3(mPosition.x + _vector3.x, mPosition.y + _vector3.y, mPosition.z + _vector3.z));
	}
}

void Transform::Translate(XMFLOAT3 _vector3, Transform * _relativeObject)
{
	if(nullptr == _relativeObject)
	{
		return;
	}

	// 오브젝트의 로컬 좌표계 기준으로 이동 -> 오브젝트 회전 행렬을 이용하여 기저를 회전 후 이동
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

		// Local 회전이므로 먼저 Local 좌표계를 기준으로 변환을 진행하고, World 회전을 적용한다.
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
		// Local 회전이므로 먼저 Local 좌표계를 기준으로 변환을 진행하고, World 회전을 적용한다.
		rotationMatrix = rotater * XMLoadFloat4x4(&mWorldRotationMatrix);
	}
	else
	{
		// 월드 좌표계 기준으로 회전
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
	// 자기 자신이 부모로 넘겨저 왔을 때,
	if (this == _parent)
	{
		return;
	}

	// 같은 부모일 때,
	if (mParent == _parent)
	{
		return;
	}

	// Parent Transform을 nullptr로 넘겨주었을 때.
	if (nullptr == _parent)
	{
		// 부모에서 자기 자신을 지운다
		// Remove 함수는 일치하는 모든 요소를 제거한다. 지워진 요소는 앞으로 당겨진다.
		// 실제로는 뒤쪽의 요소를 앞쪽으로 덮어써 버리는 과정을 거치고나서 뒤쪽의 영역을 무효화함.
		// 이렇게 지워진 채워진 임의 영역은 erase를 이용해서 지워줘야한다.
		// 아래 함수는 std::remove의 결과는 임의로 채워진 영역의 반복자가 반환된 영역부터 컨테이너의 끝 영역까지(end()) erase함수를 이용해서 지워주는 역할을 한다.
		// mComponents.erase(std::remove(mComponents.begin(), mComponents.end(), _component), mComponents.end());

		mParent->mChilds.erase(std::remove(mParent->mChilds.begin(), mParent->mChilds.end(), this), mParent->mChilds.end());
		mParent = nullptr;

		// Update Local Scale, Rotation, Translation Matrix
		UpdateLocalScaleMatrix();
		UpdateLocalRotationMatrix();
		UpdateLocalTranslationMatrix();

		return;
	}
	
	// 부모가 있을 때, 다른 부모 트랜스폼으로 지정할 경우.
	if (mParent)
	{
		// 원래 부모에서 자신을 지운다.
		mParent->mChilds.erase(std::remove(mParent->mChilds.begin(), mParent->mChilds.end(), this), mParent->mChilds.end());
	}

	// 부모가 있을 때 + 부모가 없었을 경우 (초기상태)

	// 새로운 부모를 대입하고 추가한다.
	mParent = _parent;
	mParent->mChilds.emplace_back(this);

	// Update Local Scale, Rotation, Translation Matrix
	UpdateLocalScaleMatrix();
	UpdateLocalRotationMatrix();
	UpdateLocalTranslationMatrix();
}

// World Transform Matrix 갱신 (Local은 그대로)
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

// 부모가 이동하거나, Local Scale이 변경될 때, 호출되는 함수.
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
		// Parent Transform이 없을 경우, Local Scale과 World Scale은 같다.
		mScale = mLocalScale;
		XMStoreFloat4x4(&mWorldScaleMatrix, localScaleMatrix);
	}
}

//  부모가 이동하거나, Local Rotation이 변경될 때, 호출되는 함수.
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
		// Parent Transform이 없을 경우, Local Rotatiom과 World Rotation은 같다.
		mRotation = mLocalRotation;
		XMStoreFloat4x4(&mWorldRotationMatrix, localRotationMatrix);
	}
}

//  부모가 이동하거나, Local Position이 변경될 때, 호출되는 함수.
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
		// Parent Transform이 없을 경우, Local Position과 World Position 는 같다.
		mPosition = mLocalPosition;
		XMStoreFloat4x4(&mWorldTranslationMatrix, localTranslationMatrix);
	}
}

// World Scale이 변경될 때, 호출되는 함수. (혹은 Parent Transform이 변경될 때)
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
		// Parent Transform이 없을 경우, Local Scale과 World Scale은 같다.
		mLocalScale = mScale;
		XMStoreFloat4x4(&mLocalScaleMatrix, worldScaleMatrix);
	}
}

// World Rotation이 변경될 때, 호출되는 함수. (혹은 Parent Transform이 변경될 때)
void Transform::UpdateLocalRotationMatrix()
{
	XMMATRIX worldRotationMatrix = XMLoadFloat4x4(&mWorldRotationMatrix);
	
	if (mParent)
	{
		XMMATRIX parentInverseRotationMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mParent->mWorldRotationMatrix));
		XMMATRIX localRotationMatrix = worldRotationMatrix * parentInverseRotationMatrix;

		XMFLOAT3 rotation = XMFLOAT3();
		Math::DecomposeRollPitchYawFromMatrix(localRotationMatrix, rotation.x, rotation.y, rotation.z);

		// Parent Transform이 없을 경우, Local Rotaion과 World Rotation은 같다.
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

// World Position이 변경될 때, 호출되는 함수. (혹은 Parent Transform이 변경될 때)
void Transform::UpdateLocalTranslationMatrix()
{
	XMMATRIX worldTranslationMatrix = XMLoadFloat4x4(&mWorldTranslationMatrix);

	if (mParent)
	{
		XMMATRIX parentInverseTranslationMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mParent->mWorldTranslationMatrix));
		XMMATRIX localTranslationMatrix = worldTranslationMatrix * parentInverseTranslationMatrix;

		// Parent Transform이 없을 경우, Local Position과 World Position은 같다.
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
	// 부모의 Transform Matrix 가져와서, Child의 Local Matrix 이용해서 World Matrix를 구함.
	// World Position, Rotation, Scale 값이 바뀔 수 있음
	// Grand Child도 갱신
	// 문제, Child를 갱신하기 위해서 행렬곱을 해야하고 Decompose를 해야하는데, 이게 잘 동작할 것인지?
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
