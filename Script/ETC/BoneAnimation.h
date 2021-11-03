#pragma once

#include "KeyFrame.h"

// � �ִϸ��̼��� Ư�� ������ ���� �ڷᱸ��
// �������, IDLE ������ �Ȳ�ġ �������� �ð��� ���� ������ ��.
struct BoneAnimation
{
public:

	BoneAnimation() = default;
	~BoneAnimation() = default;

	// ������ �����̴� ù �ð� ��ȯ
	float GetStartTime()const;

	// ������ ���������� �����̴� �ð� ��ȯ
	float GetEndTime()const;

	void Interpolate(float _t, DirectX::XMFLOAT4X4& _M)const;

public:

	// Key Frame�� �迭�� ��� ���� ������ ���� ������ ���� �迭��. 
	// (�ð��� t�� ��, ��� ������ ������ ���� ������ ����.)
	std::vector<KeyFrame> Keyframes;

	// ��� Bone�� ���� Animation���� �ľ��ϱ� ����.
	std::string Name;

	UINT PositionKeyCount = 0;
};