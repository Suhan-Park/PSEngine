#ifndef _SKINNED_MESH_RENDERER_H_
#define _SKINNED_MESH_RENDERER_H_

#include "MeshRenderer.h"
#include "AnimationClip.h"

class SkinnedMeshRenderer final : public MeshRenderer
{
public:

	SkinnedMeshRenderer() = default;
	virtual ~SkinnedMeshRenderer() = default;

private:

	SkinnedMeshRenderer(const SkinnedMeshRenderer &_rhs) = delete;
	SkinnedMeshRenderer& operator = (const SkinnedMeshRenderer &_rhs) = delete;
	SkinnedMeshRenderer(SkinnedMeshRenderer &&_rhs) = delete;
	SkinnedMeshRenderer& operator = (SkinnedMeshRenderer &&_rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

public:

	UINT BoneCount()const;

	// ���� �������� �ִϸ��̼��� Start, End Time�� �����´�.
	float GetAnimationClipStartTime()const;
	float GetAnimationClipEndTime()const;

	void SetBoneHierarchy(std::vector<Bone>& _boneHierarchy);
	void SetAnimationClips(std::unordered_map<std::string, AnimationClip> _animationClips);

	// ���� �������� �ִϸ��̼��� Final Transform�� ����Ѵ�.
	void CalculateFinalTransforms(float _timePos,
		std::vector<DirectX::XMFLOAT4X4>& _finalTransforms)const;

	void SetAnimationClip(std::string _name);

private:

	std::unordered_map<std::string, AnimationClip> mAnimationClips;

	class Animator* mAnimator = nullptr;

	// ���� �������� Animation Clip
	struct AnimationClip* mAnimationClip = nullptr;

	std::vector<Bone> mBoneHierarchy;
	std::vector<XMFLOAT4X4> mFinalTransforms;

	std::unique_ptr<UploadBuffer<SkinnedConstants>> mSkinningCB = nullptr;

	float t = 0;
};

#endif