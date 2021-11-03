#include "SkinnedMeshRender.h"

#include "Animator.h"
#include "AnimationClip.h"

void SkinnedMeshRenderer::Awake()
{
	Microsoft::WRL::ComPtr<ID3D12Device> device = D3DApplication::Instance()->Device();
	mSkinningCB = std::make_unique<UploadBuffer<SkinnedConstants>>(device.Get(), 1, true);
	mSkinningFlag = true;

	MeshRenderer::Awake();
}

void SkinnedMeshRenderer::Update(const float _deltaTime)
{	
	// Bone Transform, Final Transform, 상수 버퍼 True로 설정
	CalculateFinalTransforms(0.25f, mFinalTransforms);

	SkinnedConstants skinnedConstants;
	std::copy(
		std::begin(mFinalTransforms),
		std::end(mFinalTransforms),
		&skinnedConstants.BoneTransforms[0]);

	mSkinningCB->CopyData(0, skinnedConstants);

	MeshRenderer::Update(_deltaTime);
}

void SkinnedMeshRenderer::FixedUpdate(const float _fixedDltaTime)
{
}

void SkinnedMeshRenderer::Draw(const float _deltaTime)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = D3DApplication::Instance()->CommandList();
	auto SkinningCB = mSkinningCB->Resource();;
	commandList->SetGraphicsRootConstantBufferView((UINT)ROOT_SIGNATURE_ID::SKINNED, SkinningCB->GetGPUVirtualAddress());
	
	MeshRenderer::Draw(_deltaTime);
}

void SkinnedMeshRenderer::Destroy()
{
	MeshRenderer::Destroy();
}

UINT SkinnedMeshRenderer::BoneCount() const
{
	return mBoneHierarchy.size();
}

float SkinnedMeshRenderer::GetAnimationClipStartTime() const
{
	return mAnimationClip->GetAnimationClipStartTime();
}

float SkinnedMeshRenderer::GetAnimationClipEndTime() const
{
	return mAnimationClip->GetAnimationClipEndTime();
}

void SkinnedMeshRenderer::SetBoneHierarchy(std::vector<Bone>& _boneHierarchy)
{
	mBoneHierarchy = _boneHierarchy;
	mFinalTransforms.resize(BoneCount());
}

void SkinnedMeshRenderer::SetAnimationClips(std::unordered_map<std::string, AnimationClip> _animationClips)
{
	mAnimationClips = _animationClips;
}

void SkinnedMeshRenderer::CalculateFinalTransforms(float _timePos, std::vector<DirectX::XMFLOAT4X4>& _finalTransforms) const
{
	if (nullptr == mAnimationClip)
	{
		return;
	}

	// 해당 클립의 모든 Bone들을 주어진 시간에 맞추어 보간한다.
	auto clip = mAnimationClip;
	UINT numBones = BoneCount();

	numBones = clip->BoneAnimations.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);
	clip->Interpolate(_timePos, toParentTransforms);

	//
	// Bone Hierarchy를 훑으면서 모든 뼈대를 Root 공간으로 변환한다.
	//

	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// Root Bone의 색인은 0이다. Root Bone에는 부모가 없으므로, 
	// Root Bone의 Root Transform은 그냥 자신의 Local Bone Transform이다.
	toRootTransforms[0] = toParentTransforms[0];

	/// FBX SDK
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&mBoneHierarchy[i].OffsetTransform);
		XMMATRIX toRoot = XMLoadFloat4x4(&toParentTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&_finalTransforms[i], XMMatrixTranspose(finalTransform));
	}

	/// ASSIMP
	/*
	for (UINT i = 1; i < numBones; ++i)
	{
		int parentIndex = mBoneHierarchy[i].ParentIndex;
		XMMATRIX animation = XMLoadFloat4x4(&toParentTransforms[i]);
		XMMATRIX parent = XMLoadFloat4x4(&toRootTransforms[parentIndex]);
		XMMATRIX parentToRoot = animation * parent;

		XMStoreFloat4x4(&toRootTransforms[i], parentToRoot);
	}

	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&mBoneHierarchy[i].OffsetTransform);
		XMMATRIX toRoot = XMLoadFloat4x4(&mBoneHierarchy[i].RootTransform) * XMLoadFloat4x4(&toRootTransforms[i]);
		
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&_finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
	*/
}


void SkinnedMeshRenderer::SetAnimationClip(std::string _name)
{
	mAnimationClip = &mAnimationClips[_name];
}
