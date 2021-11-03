#include "ModelImporter.h"
#include "assimp/DefaultIOSystem.h"

#include "D3DApplication.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "SkinnedMeshRender.h"
#include "MeshFilter.h"

#include "AnimationClip.h"
#include "BoneAnimation.h"
#include "KeyFrame.h"
#include "PrimitiveGeometry.h"

// Assimp, OpenFBX ...

void ModelImporter::Initialize()
{
	mImporter = std::make_unique<Assimp::Importer>();
	mIOSystem = std::make_unique<Assimp::DefaultIOSystem>();
	mImporter->SetIOHandler(mIOSystem.get());

	mImporter->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

// GameObject를 인자로 받아, 파일 처리를 하면서, 그에 해당하는 컴포넌트 부착
void ModelImporter::ReadFile(GameObject* _gameObject, const std::wstring _filePath, const std::wstring _fileName)
{
	std::string path;
	path.assign(_filePath.begin(), _filePath.end());

	const aiScene *scene = mImporter->ReadFile(path,
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_LimitBoneWeights |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace);

	assert(scene != NULL);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<Subset> subsets;
	std::vector<Material*> materials;

	// 텍스처 저장 (저장 안하고 가져오는 방법 아직 모름...)
	WriteTexture(scene, _filePath, _fileName);
	ReadMaterial(scene, _filePath, &materials);

	MeshRenderer* renderer = nullptr;
	// 0번 인덱스가 Bone을 가지고 있다면, 캐릭터라고 간주한다.
	if (scene->mMeshes[0]->HasBones())
	{
		std::vector<Bone> boneHierarchy;
		ReadBoneHierarchy(scene, scene->mRootNode, -1, -1, &boneHierarchy); // -1 : root
		ReadMeshData(scene, scene->mRootNode, &vertices, &indices, &subsets, &boneHierarchy);

		renderer = new SkinnedMeshRenderer();
		SkinnedMeshRenderer* skinnedMeshRenderer = dynamic_cast<SkinnedMeshRenderer*>(renderer);
		skinnedMeshRenderer->SetBoneHierarchy(boneHierarchy);
	}
	else
	{
		ReadMeshData(scene, scene->mRootNode, &vertices, &indices, &subsets);
		renderer = new MeshRenderer();
	}

	MeshFilter* meshFilter = new MeshFilter(vertices, indices);
	_gameObject->AttachComponent(meshFilter);

	for (auto mat : materials)
	{
		renderer->AttachMaterial(mat);
	}
	_gameObject->AttachComponent(renderer);
}

void ModelImporter::ReadAnimationFile(std::unordered_map<std::string, AnimationClip>* _animationClips, const std::wstring _filePath, const std::wstring _fileName)
{
	std::string path;
	path.assign(_filePath.begin(), _filePath.end());

	const aiScene *scene = mImporter->ReadFile(path,
		aiProcess_ConvertToLeftHanded |
		aiProcess_LimitBoneWeights);

	assert(scene != NULL);

	std::vector<Bone> boneHierarchy;
	ReadBoneHierarchy(scene, scene->mRootNode, -1, -1, &boneHierarchy); // -1 : root
	
	for (UINT i = 0; i < scene->mNumAnimations; i++)
	{
		aiAnimation* aiAnimation = scene->mAnimations[i];

		AnimationClip anim;

		// 애니메이션 클립 하나를 생성한다.
		//  [A Animation Clip] -> [Bone Animations] / [A Bone Animation] -> [Keyframes]
		ReadAnimationClip(scene, aiAnimation, anim);
		_animationClips->insert({ anim.Name, anim });
	}
}

void ModelImporter::ReadMaterial(const aiScene *_scene, const std::wstring _filePath, std::vector<Material*>* _materials)
{
	std::string folderPath = GetDirectoryPath(_filePath);

	// Material 정보만 읽음.
	for (UINT i = 0; i < _scene->mNumMaterials; i++)
	{
		aiMaterial* aiMat = _scene->mMaterials[i];

		Material* mat = new Material(aiMat->GetName().C_Str());

		aiColor3D color;

		// Diffuse
		aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		//mat->SetAlbedo(XMFLOAT4(color.r, color.g, color.b, 1.0f));

		// Ambient
		aiMat->Get(AI_MATKEY_COLOR_AMBIENT, color);
		mat->SetAmbient(XMFLOAT4(color.r, color.g, color.b, 1.0f));

		// Specular
		aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color);
		mat->SetSpecular(XMFLOAT4(color.r, color.g, color.b, 1.0f));

		// Emissive는 없음...
		aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color);

		aiString file;

		/*
		aiTexture 데이터 이용해서 바로 SRV 생성하려고 하였으나 잘 안되어서 우선 Write->Read 방식으로 진행...
		if (_scene->HasTextures())
		{
			for (UINT i = 0; i < _scene->mNumMaterials; i++)
			{
				aiMaterial* material = _scene->mMaterials[i];
				aiString materialName;
				aiReturn ret;

				ret = material->Get(AI_MATKEY_NAME, materialName);
				if (ret != AI_SUCCESS)
				{
					materialName = "";
				}

				int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);
				aiString textureName;

				if (numTextures > 0)
				{
					ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName);
					const aiTexture* aiTexture = _scene->GetEmbeddedTexture(textureName.C_Str());

					const UINT rowPitch = aiTexture->mWidth * sizeof(aiTexel);
					const UINT slicePitch = rowPitch * aiTexture->mHeight;
					Texture* texture = new Texture();
					texture->CreateTexureFromPData(aiTexture->pcData, rowPitch, slicePitch, aiTexture->mWidth, aiTexture->mHeight);
				}

				numTextures = material->GetTextureCount(aiTextureType_NORMALS);
				if (numTextures > 0)
				{
					ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), textureName);
					_scene->GetEmbeddedTexture(textureName.C_Str());
					const aiTexture* aiTexture = _scene->GetEmbeddedTexture(textureName.C_Str());

					const UINT rowPitch = aiTexture->mWidth * sizeof(aiTexel);
					const UINT slicePitch = rowPitch * aiTexture->mHeight;
					Texture* texture = new Texture();
					texture->CreateTexureFromPData(aiTexture->pcData, rowPitch, slicePitch, aiTexture->mWidth, aiTexture->mHeight);
				}

				numTextures = material->GetTextureCount(aiTextureType_SPECULAR);
				if (numTextures > 0)
				{
					ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), textureName);
					_scene->GetEmbeddedTexture(textureName.C_Str());
					const aiTexture* aiTexture = _scene->GetEmbeddedTexture(textureName.C_Str());

					const UINT rowPitch = aiTexture->mWidth * sizeof(aiTexel);
					const UINT slicePitch = rowPitch * aiTexture->mHeight;
					Texture* texture = new Texture();
					texture->CreateTexureFromPData(aiTexture->pcData, rowPitch, slicePitch, aiTexture->mWidth, aiTexture->mHeight);
				}
			}
		}
		*/

		if (_scene->HasTextures())
		{
			aiString materialName;
			aiReturn ret;


			ret = aiMat->Get(AI_MATKEY_NAME, materialName);
			if (ret != AI_SUCCESS)
			{
				materialName = "";
			}

			int numTextures = aiMat->GetTextureCount(aiTextureType_DIFFUSE);
			aiString textureName;

			if (numTextures > 0)
			{
				aiMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName);

				std::string name = textureName.C_Str();
				std::string filePath = GetTextureFilePath(name, folderPath) + ".png";
				std::wstring wFilePath;
				wFilePath.assign(filePath.begin(), filePath.end());

				Texture* diffuseMap = new Texture("diffuse", wFilePath);
				diffuseMap->CreateTextureFromWICFileFormat();
				mat->AttachDiffuseMap(diffuseMap);
			}

			numTextures = aiMat->GetTextureCount(aiTextureType_NORMALS);
			if (numTextures > 0)
			{
				aiMat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), textureName);

				std::string name = textureName.C_Str();
				std::string filePath = GetTextureFilePath(name, folderPath) + ".png";
				std::wstring wFilePath;
				wFilePath.assign(filePath.begin(), filePath.end());

				Texture* normalMap = new Texture("normal", wFilePath);
				normalMap->CreateTextureFromWICFileFormat();
				mat->AttachNormalMap(normalMap);
			}

			numTextures = aiMat->GetTextureCount(aiTextureType_SPECULAR);
			if (numTextures > 0)
			{
				aiMat->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), textureName);

				std::string name = textureName.C_Str();
				std::string filePath = GetTextureFilePath(name, folderPath) + ".png";
				std::wstring wFilePath;
				wFilePath.assign(filePath.begin(), filePath.end());

				Texture* specularMap = new Texture("specular", wFilePath);
				specularMap->CreateTextureFromWICFileFormat();
				mat->AttachSpecularMap(specularMap);
			}

			_materials->emplace_back(mat);
		}
	}
}

void ModelImporter::ReadMeshData(const aiScene * _scene, const aiNode * _node, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets)
{
	const UINT numOfMeshes = _node->mNumMeshes;

	for (UINT i = 0; i < numOfMeshes; i++)
	{
		Subset subset;

		if (nullptr == _node->mMeshes)
		{
			break;
		}

		// Node에는 Mesh의 번호가 저장되어 있음.
		UINT index = _node->mMeshes[i];

		// Mesh 정보는 Scene에 있음
		aiMesh* mesh = _scene->mMeshes[index];

		// Material도 Scene에 저장되어 있음.
		// Mesh안에 필요한 Material Index 값이 저장되어 있어서 아래처럼 불러온다.
		aiMaterial* material = _scene->mMaterials[mesh->mMaterialIndex];

		// 여러 Mesh가 있을 수 있으며, 각 Mesh의 인덱스는 서로 독립적으로 존재하므로, 하나로 합치기 위해 인덱스 번호를 추가해준다.
		// 0번 메시(v1,v2,v3)를 구성하는 인덱스가 0,1,2이면 1번 메시(v1',v2',v3')를 구성하는 인덱스도 0,1,2일 수 있음.
		UINT startVertex = _vertices->size();
		UINT startIndex = _indices->size();
		for (UINT v = 0; v < mesh->mNumVertices; v++)
		{
			Vertex vertex;

			vertex.Position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

			if (mesh->HasNormals())
			{
				vertex.Normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
			}

			if (mesh->HasTangentsAndBitangents())
			{
				vertex.TangentU = XMFLOAT3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
			}

			if (mesh->HasTextureCoords(0))
			{
				vertex.TexCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			}

			_vertices->emplace_back(vertex);
		}

		for (UINT f = 0; f < mesh->mNumFaces; f++)
		{
			// 복사생성자 호출 안함.
			aiFace& face = mesh->mFaces[f];

			// Face를 구성하는 인덱스를 저장한다.
			for (UINT k = 0; k < face.mNumIndices; k++)
			{
				_indices->push_back(face.mIndices[k]);
				_indices->back() += startVertex;
			}
		}

		subset.MaterialName = material->GetName().C_Str();
		subset.IndexCount = _indices->size() - subset.StartIndexLocation;
		subset.StartIndexLocation = startIndex;
		subset.BaseVertexLocation = startVertex;
		_subsets->emplace_back(subset);
	}

	for (UINT c = 0; c < _node->mNumChildren; c++)
	{
		ReadMeshData(_scene, _node->mChildren[c], _vertices, _indices, _subsets);
	}
}

// Skeletal Mesh를 불러올 때, 호출한다. (e.g 캐릭터)
void ModelImporter::ReadMeshData(const aiScene * _scene, const aiNode * _node, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets, std::vector<Bone>* _bones)
{
	const UINT numOfMeshes = _node->mNumMeshes;
	for (UINT i = 0; i < numOfMeshes; i++)
	{
		Subset subset;

		if (nullptr == _node->mMeshes)
		{
			break;
		}

		// Node에는 Mesh의 번호가 저장되어 있음.
		UINT index = _node->mMeshes[i];

		// Mesh 정보는 Scene에 있음
		aiMesh* mesh = _scene->mMeshes[index];

		// Material도 Scene에 저장되어 있음.
		// Mesh안에 필요한 Material Index 값이 저장되어 있어서 아래처럼 불러온다.
		aiMaterial* material = _scene->mMaterials[mesh->mMaterialIndex];

		// 여러 Mesh가 있을 수 있으며, 각 Mesh의 인덱스는 서로 독립적으로 존재하므로, 하나로 합치기 위해 인덱스 번호를 추가해준다.
		// 0번 메시(v1,v2,v3)를 구성하는 인덱스가 0,1,2이면 1번 메시(v1',v2',v3')를 구성하는 인덱스도 0,1,2일 수 있음.
		UINT startVertex = _vertices->size();
		UINT startIndex = _indices->size();
		for (UINT v = 0; v < mesh->mNumVertices; v++)
		{
			Vertex vertex;

			vertex.Position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

			if (mesh->HasNormals())
			{
				vertex.Normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
			}

			if (mesh->HasTangentsAndBitangents())
			{
				vertex.TangentU = XMFLOAT3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
			}

			if (mesh->HasTextureCoords(0))
			{
				vertex.TexCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
			}

			_vertices->emplace_back(vertex);
		}
		for (UINT f = 0; f < mesh->mNumFaces; f++)
		{
			// 복사생성자 호출 안함.
			aiFace& face = mesh->mFaces[f];

			// Face를 구성하는 인덱스를 저장한다.
			for (UINT k = 0; k < face.mNumIndices; k++)
			{
				_indices->push_back(face.mIndices[k]);
				_indices->back() += startVertex;
			}
		}

		// Mesh의 Bone과 Vertex를 연결
		if (mesh->HasBones())
		{
			ReadBoneWeightsForVertices(_scene, mesh, _vertices, _bones);
		}

		subset.MaterialName = material->GetName().C_Str();
		subset.IndexCount = _indices->size() - subset.StartIndexLocation;
		subset.StartIndexLocation = startIndex;
		subset.BaseVertexLocation = startVertex;
		_subsets->emplace_back(subset);
	}

	for (UINT c = 0; c < _node->mNumChildren; c++)
	{
		ReadMeshData(_scene, _node->mChildren[c], _vertices, _indices, _subsets, _bones);
	}
}

void ModelImporter::ReadBoneWeightsForVertices(const aiScene * _scene, aiMesh * _mesh, std::vector<Vertex>* _vertices, std::vector<Bone>* _bones)
{
	std::vector<std::vector<int>> boneIndices(_mesh->mNumVertices, std::vector<int>(4, -1));
	std::vector<std::vector<float>> boneWeights(_mesh->mNumVertices, std::vector<float>(4, 0.0f));

	for (int boneIdx = 0; boneIdx < _mesh->mNumBones; boneIdx++)
	{
		int boneID = -1;
		std::string boneName = _mesh->mBones[boneIdx]->mName.C_Str();
		
		for (int i = 0; _bones->size(); i++)
		{
			if (_bones->at(i).Name == boneName)
			{
				boneID = i;
				XMMATRIX transform = XMLoadFloat4x4((const XMFLOAT4X4*)&_mesh->mBones[boneIdx]->mOffsetMatrix);
				XMStoreFloat4x4(&_bones->at(i).OffsetTransform, XMMatrixTranspose(transform));
				break;
			}
		}
		
		auto weights = _mesh->mBones[boneIdx]->mWeights;
		int numWeights = _mesh->mBones[boneIdx]->mNumWeights;

		for (int weightIdx = 0; weightIdx < numWeights; weightIdx++)
		{
			int vertexId = weights[weightIdx].mVertexId;
			float weight = weights[weightIdx].mWeight;

			for (UINT k = 0; k < 4; k++)
			{
				_vertices->at(vertexId).BoneIndices[k] = boneID;
				_vertices->at(vertexId).BoneWeights[k] = weight;
			}
		}
	}
}

// Root index : -1 
void ModelImporter::ReadBoneHierarchy(const aiScene *_scene, const aiNode * _node, int _index, int _parent, std::vector<Bone>* _bones)
{
	// Mesh가 없는 Node를 Bone으로 간주하며, 루트노드는 제외한다.
	if (_node->mNumMeshes < 1 && _node != _scene->mRootNode)
	{
		Bone bone;

		// 자신의 Bone 인덱스 저장
		bone.Index = _index;

		// 부모 Bone 인덱스 저장
		bone.ParentIndex = _parent;

		// Bone의 이름 저장 (e.g. pelvis)
		bone.Name = _node->mName.C_Str();

		// Transformation 저장
		XMMATRIX matrix = XMLoadFloat4x4((const XMFLOAT4X4*)&_node->mTransformation);

		// Assimp는 열우선이므로 행우선으로 바꾸어준다.
		matrix = XMMatrixTranspose(matrix);

		XMStoreFloat4x4(&bone.LocalTransform, matrix);
		XMStoreFloat4x4(&bone.LocalOffsetTransform, XMMatrixInverse(nullptr, matrix));
 
		XMMATRIX parent = XMMatrixIdentity();
		if (_parent > -1)
		{
			parent = XMLoadFloat4x4(&_bones->at(_parent).RootTransform);
		}

		matrix = XMMatrixMultiply(matrix, parent);
		XMStoreFloat4x4(&bone.RootTransform, matrix);			                    // i번째 Bone에서 Root로의 변환 행렬
		XMStoreFloat4x4(&bone.OffsetTransform, XMMatrixInverse(nullptr, matrix));	// Root Bone에서 i번째 Bone으로의 변환 행렬
		_bones->emplace_back(bone);
	}

	// mNumChildren : 어느 한 노드의 자식 개수
	// aiNode는 Material이 포함된 Mesh Bone 정보를 가지고 있다.
	const UINT numOfChild = _node->mNumChildren;

	for (int i = 0; i < numOfChild; i++)
	{
		ReadBoneHierarchy(_scene, _node->mChildren[i], _bones->size(), _index, _bones);
	}
}

// 모든 Bone이 아닌, 활성화된 Bone에 대해서만 기록될 수 있으므로, 다시 Node를 순회하면서, 기록되지 않은 Bone에 대해서 원래의 Transform 값으로 채워줘야한다.
void ModelImporter::ReadAnimationClip(const aiScene *_scene, aiAnimation* _aiAnimation, AnimationClip& _animationClip)
{
	AnimationClip clip;

	UINT keyFrameCount = 0;
	for (UINT i = 0; i < _aiAnimation->mNumChannels; i++)
	{
		aiNodeAnim* animNode = _aiAnimation->mChannels[i];

		BoneAnimation temp;
		temp.Name = animNode->mNodeName.data;

		// Key Frame Count를 최대로 잡는다.
		keyFrameCount = std::max(animNode->mNumPositionKeys, animNode->mNumRotationKeys);
		keyFrameCount = std::max(keyFrameCount, animNode->mNumScalingKeys);

		temp.PositionKeyCount = animNode->mNumPositionKeys;

		// 하나의 Bone Animation을 구성하는 Key Frame들을 가져와서 추가한다.
		for (UINT k = 0; k < keyFrameCount; k++)
		{
			KeyFrame keyFrame;
			bool isFound = false;

			float t = temp.Keyframes.size();

			// Position
			if (k < animNode->mNumPositionKeys)
			{
				aiVectorKey key = animNode->mPositionKeys[k];
				keyFrame.Position = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
				keyFrame.TimePosition = animNode->mPositionKeys[k].mTime;
			
				isFound = true;
			}

			// Rotation
			if (k < animNode->mNumRotationKeys)
			{
				aiQuatKey key = animNode->mRotationKeys[k];
				keyFrame.Quaternion = XMFLOAT4(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
				keyFrame.TimePosition = animNode->mRotationKeys[k].mTime;

				isFound = true;
			}

			if (isFound)
			{
				// 하나라도 데이터가 있다면 키프레임 데이터 저장
				temp.Keyframes.emplace_back(keyFrame);
			}
		}

		clip.Duration = std::max(clip.Duration, temp.Keyframes.back().TimePosition);
		clip.BoneAnimations.push_back(temp);
	}

	clip.Name = "Dance";
	clip.FrameRate = _aiAnimation->mTicksPerSecond;
	clip.FrameCount = keyFrameCount;

	std::vector<BoneAnimation> boneAnimation;
	ReadKeyFrames(_scene, _scene->mRootNode, clip.FrameCount,-1, -1,  &clip.BoneAnimations, &boneAnimation);

	clip.BoneAnimations = boneAnimation;

	_animationClip = clip;
}

void ModelImporter::ReadKeyFrames(const aiScene * _scene, const aiNode* _node, UINT _frameCount, int  _index, int _parent, std::vector<BoneAnimation>* _in, std::vector<BoneAnimation>* _out)
{
	if (_node != _scene->mRootNode)
	{
		// 애니메이션의 본 개수와 본의 개수를 맞춘다.
		BoneAnimation boneAnimation;

		for (UINT i = 0; i < _in->size(); i++)
		{
			// fbx 확장자에서 사용할 때 Mixamo 애니메이션이 애니메이션 본이 실제 본의 이름과 다름...
			// dae 확장자는 그렇지 않음.
			std::string name = _in->at(i).Name;
			int split = _in->at(i).Name.find("_");
			if (split > 0)
			{
				name = _in->at(i).Name.substr(0, split);
			}

			if (name == _node->mName.C_Str())
			{
				boneAnimation = _in->at(i);
				_out->push_back(boneAnimation);
				break;
			}
		}

		if (boneAnimation.Keyframes.size() == 0)
		{
			std::cout << _node->mName.C_Str() << std::endl;
			BoneAnimation temp = _in->at(0);
			for (int k = 0; k < _frameCount; k++)
			{
				KeyFrame keyFrame;
				keyFrame.TimePosition = temp.Keyframes[k].TimePosition;

				XMMATRIX transform = XMLoadFloat4x4((const XMFLOAT4X4*)&_node->mTransformation);
				
				// Assimp는 열우선이므로 행우선으로 바꾸어준다.
				transform = XMMatrixTranspose(transform);

				XMVECTOR scale;
				XMVECTOR rotation;
				XMVECTOR translation;
				XMMatrixDecompose(&scale, &rotation, &translation, transform);
				XMStoreFloat3(&keyFrame.Scale, scale);
				XMStoreFloat4(&keyFrame.Quaternion, rotation);
				XMStoreFloat3(&keyFrame.Position, translation);

				temp.Keyframes[k] = keyFrame;
			}
			boneAnimation = temp;

			boneAnimation.Name = _node->mName.C_Str();
			_out->push_back(boneAnimation);
		}

	}
	for (UINT i = 0; i < _node->mNumChildren; i++)
	{
		ReadKeyFrames(_scene, _node->mChildren[i], _frameCount, _out->size(), _index, _in, _out);
	}

}

void ModelImporter::WriteMaterial(const aiScene *_scene, std::vector<Material*> _materials, std::wstring _filePath, const bool _isOverwrite)
{
	// https://docs.microsoft.com/ko-kr/cpp/standard-library/filesystem?view=msvc-160
	using namespace std::experimental::filesystem::v1;

	if (false == _isOverwrite)
	{
		// 이미 파일이 있는 경우 XML을 따로 저장하지 않음.
		if (std::experimental::filesystem::exists(path(_filePath)))
		{
			return;
		}
	}

	// 폴더 경로 가지고 옴
	std::string folderPath = GetDirectoryPath(_filePath);
	std::wstring wFolderPath;
	wFolderPath.assign(folderPath.begin(), folderPath.end());

	std::string fileName = GetFileName(_filePath);

	path p(folderPath);
	create_directory(p);

	tinyxml2::XMLDocument* document = new tinyxml2::XMLDocument();
	tinyxml2::XMLDeclaration* decl = document->NewDeclaration();
	document->LinkEndChild(decl);

	// Root Attribute
	tinyxml2::XMLElement* root = document->NewElement("Materials");

	root->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	root->SetAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
	document->LinkEndChild(root);

	// Material을 XML로 저장
	for (auto mat : _materials)
	{
		// Material Element 추가
		tinyxml2::XMLElement* node = document->NewElement("Material");
		root->LinkEndChild(node);

		tinyxml2::XMLElement* element = nullptr;

		// 이름 저장
		element = document->NewElement("Name");
		element->SetText(mat->Name().c_str());
		node->LinkEndChild(element);

		// 텍스처 파일 저장
		element = document->NewElement("DiffuseMap");
		element->SetText(WriteTexture(_scene, wFolderPath, mat->DiffuseMap()->File().c_str()).c_str());
		node->LinkEndChild(element);

		element = document->NewElement("NormalMap");
		element->SetText(WriteTexture(_scene, wFolderPath, mat->NormalMap()->File().c_str()).c_str());
		node->LinkEndChild(element);

		element = document->NewElement("SpecularMap");
		element->SetText(WriteTexture(_scene, wFolderPath, mat->SpecularMap()->File().c_str()).c_str());
		node->LinkEndChild(element);

		// 색상 저장
		element = document->NewElement("Diffuse");
		element->SetAttribute("R", mat->GetAlbedo().x);
		element->SetAttribute("G", mat->GetAlbedo().y);
		element->SetAttribute("B", mat->GetAlbedo().z);
		element->SetAttribute("A", mat->GetAlbedo().w);
		node->LinkEndChild(element);

		element = document->NewElement("Ambient");
		element->SetAttribute("R", mat->GetAmbient().x);
		element->SetAttribute("G", mat->GetAmbient().y);
		element->SetAttribute("B", mat->GetAmbient().z);
		element->SetAttribute("A", mat->GetAmbient().w);
		node->LinkEndChild(element);

		element = document->NewElement("Specular");
		element->SetAttribute("R", mat->GetSpecular().x);
		element->SetAttribute("G", mat->GetSpecular().y);
		element->SetAttribute("B", mat->GetSpecular().z);
		element->SetAttribute("A", mat->GetSpecular().w);
		node->LinkEndChild(element);
	}

	document->SaveFile((folderPath + fileName).c_str());
	delete document;
}

std::string ModelImporter::WriteTexture(const aiScene *_scene, std::wstring _filePath, std::wstring _fileName)
{
	// https://docs.microsoft.com/ko-kr/cpp/standard-library/filesystem?view=msvc-160
	using namespace std::experimental::filesystem::v1;

	// 파일명이 제외된 디렉토리 경로 A/B/C/
	std::string folderPath = GetDirectoryPath(_filePath);

	/*
	// https://stackoverflow.com/questions/59258084/assimp-how-do-you-import-a-mesh-with-textures-using-any-file-format
	if (_scene->HasTextures())
	{
		for (UINT i = 0; i < _scene->mNumMaterials; i++)
		{
			aiMaterial* material = _scene->mMaterials[i];
			aiString materialName;
			aiReturn ret;

			ret = material->Get(AI_MATKEY_NAME, materialName);
			if (ret != AI_SUCCESS)
			{
				materialName  = "";
			}

			int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);
			aiString textureName;

			if (numTextures > 0)
			{
				ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureName);
				std::string textureFileName = GetFileNameWithoutExtension(_fileName)+ std::to_string(i) + "_diffuse.png";
			}

			numTextures = material->GetTextureCount(aiTextureType_NORMALS);
			if (numTextures > 0)
			{
				ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), textureName);
				std::string textureFileName = GetFileNameWithoutExtension(_fileName) + std::to_string(i) + "_normal.png";//The actual name of the texture file

			}

			numTextures = material->GetTextureCount(aiTextureType_SPECULAR);
			if (numTextures > 0)
			{
				ret = material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), textureName);
				std::string textureFileName = GetFileNameWithoutExtension(_fileName) + std::to_string(i) +"_specular.png";
			}
		}
	}
	*/

	for (UINT i = 0; i < _scene->mNumTextures; i++)
	{
		const aiTexture* texture = _scene->mTextures[i];
		// FBX에 Texture가 내장되어 있는 경우
		if (nullptr != texture)
		{
			std::string name = texture->mFilename.C_Str();
			std::string filePath = GetTextureFilePath(name, folderPath) + ".png";

			// 이미 파일이 있는 경우 텍스처를 따로 저장하지 않음.
			if (std::experimental::filesystem::exists(path(filePath)))
			{
				return "";
			}

			// 높이가 1보다 작으면, 바이트가 쭉 써있음.
			if (texture->mHeight < 1)
			{
				std::ofstream fout;
				fout.open(filePath, std::ios::out | std::ios::binary);
				if (fout)
				{
					std::ofstream fout;
					fout.open(filePath, std::ios::out | std::ios::binary);
					if (fout)
					{
						fout.write((const char*)texture->pcData, texture->mWidth);
						fout.close();
					}
				}
			}
			else
			{
				std::unique_ptr<Assimp::IOStream> outfile(mIOSystem.get()->Open(static_cast<const std::string>(filePath), static_cast<const std::string>("w")));
				Assimp::Bitmap::Save(const_cast<aiTexture *>(texture), outfile.get());
			}
		}
	}

	return "";
}

void ModelImporter::WriteMeshData(std::wstring _filePath, bool _isOverwrite)
{
	// https://docs.microsoft.com/ko-kr/cpp/standard-library/filesystem?view=msvc-160
	using namespace std::experimental::filesystem::v1;

	if (false == _isOverwrite)
	{
		if (std::experimental::filesystem::exists(path(_filePath)))
		{
			return;
		}
	}

	path p(GetDirectoryPath(_filePath));
	create_directory(p);

	std::ofstream fout;
	fout.open(_filePath, std::ios::out);

	if (fout.fail())
	{
		return;
	}

	std::vector<Bone> _bones;
	UINT size = _bones.size();
	fout << _bones.size() << '\n';
	for (auto bone : _bones)
	{
		fout << bone.Name << '\n';
		fout << bone.Index << '\n';
		fout << bone.ParentIndex << '\n';
	}


	fout.close();
}
