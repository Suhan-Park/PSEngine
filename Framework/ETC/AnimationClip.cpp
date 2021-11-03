#include "AnimationClip.h"

float AnimationClip::GetAnimationClipStartTime() const
{
	float t = FLT_MAX;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		t = std::min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetAnimationClipEndTime() const
{
	float t = 0.0f;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		t = std::max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float _t, std::vector<DirectX::XMFLOAT4X4>& _boneTransforms)
{
	T = T + _t;
	
	if (T > GetAnimationClipEndTime())
	{
		T = 0;
	}

	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		BoneAnimations[i].Interpolate(T, _boneTransforms[i]);
	}
}
