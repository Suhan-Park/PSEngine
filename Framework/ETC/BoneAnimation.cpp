#include "BoneAnimation.h"

float BoneAnimation::GetStartTime() const
{
	return Keyframes.front().TimePosition;
}

float BoneAnimation::GetEndTime() const
{
	return Keyframes.back().TimePosition;
}

void BoneAnimation::Interpolate(float _t, DirectX::XMFLOAT4X4 & _M) const
{
	// Bone이 움직일 시간에 미달하거나 초과하였으면
	// 보간을 하지 않는다.


	if (_t <= Keyframes.front().TimePosition)
	{
		XMVECTOR P = XMLoadFloat3(&Keyframes.front().Position);
		XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.front().Quaternion);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (_t >= Keyframes.back().TimePosition)
	{
		XMVECTOR P = XMLoadFloat3(&Keyframes.back().Position);
		XMVECTOR S = XMLoadFloat3(&Keyframes.back().Scale);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.back().Quaternion);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		for (UINT i = 0; i < Keyframes.size() - 1; ++i)
		{
			// 단일 관절에 대한 보간이다.
			// 현재 Key Frame과 다음 Key Frame의 움직임을 보간을 한다.
			// [Keyframe[i]] -> [?] -> [Keyframe[i+1]]을 보간하기 위해, 시간을 이용
			// [Keyframe[i].TimePosition] -> [_t] -> [Keyframe[i+1].TimePosition]
			if (_t >= Keyframes[i].TimePosition && _t <= Keyframes[i + 1].TimePosition)
			{
				float lerpPercent = (_t - Keyframes[i].TimePosition) / (Keyframes[i + 1].TimePosition - Keyframes[i].TimePosition);

				XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Position);
				XMVECTOR p1 = XMLoadFloat3(&Keyframes[i + 1].Position);

				XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				XMVECTOR s1 = XMLoadFloat3(&Keyframes[i + 1].Scale);

				XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].Quaternion);
				XMVECTOR q1 = XMLoadFloat4(&Keyframes[i + 1].Quaternion);

				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}