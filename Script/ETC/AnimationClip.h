#ifndef _ANIMATION_CLIP_H_
#define _ANIMATION_CLIP_H_

#include "BoneAnimation.h"

// Animation Clip�� Bone Animation�� �ð� ������ ���� �Ǿ� ������.
struct AnimationClip
{
public:

	AnimationClip() = default;
	~AnimationClip() = default;

	float GetAnimationClipStartTime()const;
	float GetAnimationClipEndTime()const;

	void Interpolate(float _t, std::vector<DirectX::XMFLOAT4X4>& _boneTransforms);
	std::vector<BoneAnimation> BoneAnimations;

public:

	std::string Name = "None";
	float FrameRate = 0.0f;
	float Duration = 0.0f;
	INT FrameCount = 0;
	float T = 0.0f;
};

#endif