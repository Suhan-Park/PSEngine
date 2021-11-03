#ifndef _ANIMATION_CLIP_H_
#define _ANIMATION_CLIP_H_

#include "BoneAnimation.h"

// Animation Clip은 Bone Animation이 시간 순서로 나열 되어 구성됨.
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