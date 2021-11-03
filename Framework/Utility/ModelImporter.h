#ifndef _MODEL_IMPORTER_H_
#define _MODEL_IMPORTER_H_

#include "Material.h"
#include "Vertex.h"

class GameObject;
class MeshRenderer;
class SkinnedMeshRenderer;
class MeshFilter;

struct Subset;
struct AnimationClip;
struct BoneAnimation;

class ModelImporter
{
private:

	ModelImporter() = default;
	~ModelImporter() = default;
	ModelImporter(const ModelImporter& _rhs) = delete;
	ModelImporter& operator = (const ModelImporter& _rhs) = delete;
	ModelImporter(ModelImporter&& _rhs) = delete;
	ModelImporter& operator = (ModelImporter&& _rhs) = delete;

public:

	static ModelImporter* Instance()
	{
		static ModelImporter instance;
		return &instance;
	}

public:

	void Initialize();
	void ReadFile(GameObject* _gameObject, const std::wstring _filePath, const std::wstring _fileName);
	void ReadAnimationFile(std::unordered_map<std::string, AnimationClip>* _animationClips, const std::wstring _filePath, const std::wstring _fileName);

private:

	// Material의 경우 여러 Mesh에서 공유될 수 있으므로, 간편한 바인딩을 위해 Map으로 관리한다.
	void ReadMaterial(const aiScene *_scene, const std::wstring _filePath, std::vector<Material*>* _materials);
	void ReadMeshData(const aiScene *_scene, const aiNode* _node, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets);
	void ReadMeshData(const aiScene *_scene, const aiNode* _node, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets, std::vector<Bone>*_bones);

	void ReadBoneWeightsForVertices(const aiScene* _scene, aiMesh* _mesh, std::vector<Vertex>* _vertices, std::vector<Bone>*_bones);
	
	void ReadBoneHierarchy(const aiScene *_scene, const aiNode* _node, int _index, int _parent, std::vector<Bone>*_bones);
	void ReadAnimationClip(const aiScene *_scene, aiAnimation* _aiAnimation, AnimationClip& _animationClip);
	void ReadKeyFrames(const aiScene * _scene, const aiNode* _node, UINT _frameCount, int _index, int _parent , std::vector<BoneAnimation>* _in, std::vector<BoneAnimation>* _out);

	// 불러온 데이터 따로 저장하기
	void WriteMaterial(const aiScene *_scene, std::vector<Material*> _materials, std::wstring _folderPath, const bool _isOverwrite = true);
	std::string WriteTexture(const aiScene *_scene, std::wstring _filePath, std::wstring _fileName);
	void WriteMeshData(std::wstring _filePath, bool _isOverwrite = true);

private:

	std::map<std::string, Bone> mBone;
	int m_BoneCounter = 0;

	std::unique_ptr<Assimp::Importer> mImporter = nullptr;
	std::unique_ptr<Assimp::IOSystem> mIOSystem;

};

#endif