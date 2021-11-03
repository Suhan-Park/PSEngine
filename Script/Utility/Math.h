#pragma once
#ifndef _MATH_H_
#define _MATH_H_

using namespace DirectX;

const FLOAT PI = 3.14159265359f;
const FLOAT RADIAN = 57.29578f;

class Math
{
public:

	// 0에서 1사이 값을 리턴한다.
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// A에서 B사이 값을 리턴한다.
	static float RandF(float a, float b)
	{
		return a + RandF()*(b - a);
	}

	static XMFLOAT4 EulerToQuaternion(FLOAT _pitch, FLOAT _yaw, FLOAT _roll)
	{
		FLOAT sp = Math::Sin(_pitch * 0.5f);
		FLOAT cp = Math::Cos(_pitch * 0.5f);
		FLOAT cy = Math::Cos(_yaw * 0.5f);
		FLOAT sy = Math::Sin(_yaw * 0.5f);
		FLOAT sr = Math::Sin(_roll * 0.5f);
		FLOAT cr = Math::Cos(_roll * 0.5f);

		XMFLOAT4 quaternion;
		quaternion.w = cr * cp * cy + sr * sp * sy;
		quaternion.x = sr * cp * cy - cr * sp * sy;
		quaternion.y = cr * sp * cy + sr * cp * sy;
		quaternion.z = cr * cp * sy - sr * sp * cy;

		return quaternion;
	}

	static XMFLOAT3 QuaternionToEuler(XMFLOAT4 _quaternion)
	{
		XMFLOAT3 eulerAngle;

		float sqw = _quaternion.w * _quaternion.w;
		float sqx = _quaternion.x * _quaternion.x;
		float sqy = _quaternion.y * _quaternion.y;
		float sqz = _quaternion.z * _quaternion.z;

		eulerAngle.x = atan2f(-2.0f * (_quaternion.w * _quaternion.z - _quaternion.x * _quaternion.y), (-sqx + sqy - sqz + sqw));
		eulerAngle.y = atan2f(2.0f * (_quaternion.w * _quaternion.y - _quaternion.x * _quaternion.z), (-sqx - sqy + sqz + sqw));
		eulerAngle.z = asinf(2.0f * (_quaternion.y * _quaternion.z + _quaternion.w * _quaternion.x));

		if (eulerAngle.x < 0.001f)
		{
			eulerAngle.x = 0.0f;
		}

		if (eulerAngle.y < 0.001f)
		{			   
			eulerAngle.y = 0.0f;
		}

		if (eulerAngle.z < 0.001f)
		{			   
			eulerAngle.z = 0.0f;
		}

		return eulerAngle;
	}

	static FLOAT ToDegree(const FLOAT& _radian)
	{
		return _radian * RADIAN;
	}

	static FLOAT ToRadian(const FLOAT& _degree)
	{
		return _degree / RADIAN;
	}

	static FLOAT Sin(const FLOAT& _angle)
	{
		return sin(_angle);
	}

	static FLOAT Cos(const FLOAT& _angle)
	{
		return cos(_angle);
	}

	static FLOAT Dot(const  XMFLOAT3& _lhs, const XMFLOAT3& _rhs)
	{
		return (_lhs.x * _rhs.x) + (_lhs.y * _rhs.y) + (_lhs.z * _rhs.z);
	}

	static XMFLOAT3 Cross(const XMFLOAT3& _lhs, const XMFLOAT3& _rhs)
	{
		FLOAT x = _lhs.y * _rhs.z - _lhs.z * _rhs.y;
		FLOAT y = _lhs.z * _rhs.x - _lhs.x * _rhs.z;
		FLOAT z = _lhs.x * _rhs.y - _lhs.y * _rhs.x;

		return XMFLOAT3(x, y, z);
	}

	static FLOAT Sqrt(const FLOAT& _value)
	{
		return sqrt(_value);
	}

	static XMFLOAT3 Normalize(const XMFLOAT3& _vector3)
	{
		float length = Length(_vector3);
		return XMFLOAT3(_vector3.x / length, _vector3.y / length, _vector3.z / length);
	}

	static FLOAT Length(const XMFLOAT3& _vector3)
	{
		return Sqrt(_vector3.x * _vector3.x + _vector3.y * _vector3.y + _vector3.z * _vector3.z);
	}

	static XMFLOAT4X4 Identity4x4()
	{
		static XMFLOAT4X4 identity(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return identity;
	}

	// Roll(Z) - Pitch(X) - Yaw(Y)
	static void DecomposeRollPitchYawFromMatrix(XMMATRIX& _matrix, float& _pitch, float& _yaw, float& _roll)
	{
		// http://planning.cs.uiuc.edu/node103.html#eqn:angfrommat2

		// 분자의 기호는 x축 위, 아래에 있는지 결정하고 분모는 y축 왼쪽, 오른쪽에 있을지 결정한다. (atan2 함수 사용하면 쉽게 해결 가능)

		//_pitch = atan(_matrix.r[2].m128_f32[1] / sqrt(pow(_matrix.r[2].m128_f32[0], 2) + pow(_matrix.r[2].m128_f32[2], 2)));
		//_yaw = atan(_matrix.r[2].m128_f32[0] / _matrix.r[2].m128_f32[2]);
		//_roll = atan(_matrix.r[0].m128_f32[1] / _matrix.r[1].m128_f32[1]);
	
		_pitch = static_cast<float>(atan2(-_matrix.r[2].m128_f32[1], sqrt(pow(_matrix.r[2].m128_f32[0], 2) + pow(_matrix.r[2].m128_f32[2], 2))));
		_yaw = static_cast<float>(atan2(_matrix.r[2].m128_f32[0], _matrix.r[2].m128_f32[2]));
		_roll = static_cast<float>(atan2(_matrix.r[0].m128_f32[1], _matrix.r[1].m128_f32[1]));
	}
};

#endif // !_MATH_H_
