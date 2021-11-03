#ifndef _FBX_LOADER_H_
#define _FBX_LOADER_H_

#include "Material.h"
#include "Vertex.h"

class GameObject;
class MeshRenderer;
class SkinnedMeshRenderer;
class MeshFilter;

struct Subset;
struct AnimationClip;
struct BoneAnimation;

struct BoneIndexAndWeight
{
	INT BoneIndices;
	FLOAT BoneWeight;

	bool operator < (const BoneIndexAndWeight& rhs)
	{
		return (BoneWeight > rhs.BoneWeight);
	}
};

struct CtrlPoint
{
	XMFLOAT3 Position;
	std::vector<BoneIndexAndWeight> BoneInfo;
	std::string BoneName;

	CtrlPoint()
	{
		BoneName.reserve(4);
	}

	void SortBlendingInfoByWeight()
	{
		std::sort(BoneInfo.begin(), BoneInfo.end());
	}
};

class FBXLoader
{
private:
	
	FBXLoader() = default;
	~FBXLoader() = default;
	FBXLoader(const FBXLoader& _rhs) = delete;
	FBXLoader& operator = (const FBXLoader& _rhs) = delete;
	FBXLoader(FBXLoader&& _rhs) = delete;
	FBXLoader& operator = (FBXLoader&& _rhs) = delete;

public:

	static FBXLoader* Instance()
	{
		static FBXLoader instance;
		return &instance;
	}

public:

	void Initialize();

public:

	void ReadFile(GameObject* _gameObject, const std::wstring _filePath);
	void ReadAnimationFile(std::unordered_map<std::string, AnimationClip>* _animationClips, const std::wstring _filePath);

	void ReadControlPoint(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, std::vector<CtrlPoint>* _controlPoints);
	void ReadMeshData(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, const fbxsdk::FbxMesh* _fbxMesh, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets, std::vector<CtrlPoint>* _controlPoints);
	void ReadSkeleton(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, std::vector<Bone>*_bones, std::vector<CtrlPoint>* _controlPoints);
	void ReadBoneHierarchy(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, int _index, int _parent, std::vector<Bone>*_bones);
	void ReadMaterial(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, std::vector<Material*>* _materials);
	void ReadAnimationClip(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, AnimationClip& _animationClip, std::vector<Bone>*_bones);
	void CalculateTangent(std::vector<Vertex>& _vertices, std::vector<uint16_t>& _indices);
	void ReadMaterialAttribute(fbxsdk::FbxSurfaceMaterial* _fbxMaterial, Material* _mat);
	void ReadTexture(fbxsdk::FbxSurfaceMaterial* _fbxMaterial, Material* _mat);

private:
	
	fbxsdk::FbxAMatrix GetGeometryTransformation(const fbxsdk::FbxNode* _node);

	XMFLOAT2 ReadUV(const FbxMesh* _mesh, int _controlPointIndex, int _indexCounter);
	XMFLOAT3 ReadTangent(const FbxMesh* _mesh, int _controlPointIndex, int _vertexCounter);
	XMFLOAT3 ReadBinormal(const FbxMesh* _mesh, int _controlPointIndex, int _vertexCounter);
	XMFLOAT3 ReadNormal(const FbxMesh* _mesh, int _controlPointIndex, int _vertexCounter);

	std::string HashFunction(Vertex& _vertex);

private:

	fbxsdk::FbxManager* mFbxManager = nullptr;
};

#endif // !FBX_LOADER_H_
