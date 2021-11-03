#pragma once

#include "KeyFrame.h"

// 어떤 애니메이션의 특정 관절에 대한 자료구조
// 예를들어, IDLE 동작의 팔꿈치 움직임을 시간에 따라 나열한 것.
struct BoneAnimation
{
public:

	BoneAnimation() = default;
	~BoneAnimation() = default;

	// 관절이 움직이는 첫 시간 반환
	float GetStartTime()const;

	// 관절이 마지막으로 움직이는 시간 반환
	float GetEndTime()const;

	void Interpolate(float _t, DirectX::XMFLOAT4X4& _M)const;

public:

	// Key Frame의 배열은 모두 같은 관절에 대한 정보를 가진 배열임. 
	// (시간이 t일 때, 모든 관절의 정보를 담지 않음을 말함.)
	std::vector<KeyFrame> Keyframes;

	// 어느 Bone에 대한 Animation인지 파악하기 위함.
	std::string Name;

	UINT PositionKeyCount = 0;
};