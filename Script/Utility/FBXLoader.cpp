#include "FBXLoader.h"

#include "D3DApplication.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "SkinnedMeshRender.h"
#include "MeshFilter.h"

#include "AnimationClip.h"
#include "BoneAnimation.h"
#include "KeyFrame.h"
#include "PrimitiveGeometry.h"

void FBXLoader::Initialize()
{
	mFbxManager = FbxManager::Create();

	FbxIOSettings* pIOsettings = FbxIOSettings::Create(mFbxManager, IOSROOT);
	mFbxManager->SetIOSettings(pIOsettings);
}

void FBXLoader::ReadFile(GameObject * _gameObject, const std::wstring _filePath)
{
	std::string path;
	path.assign(_filePath.begin(), _filePath.end());

	fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(mFbxManager, "");
	if (nullptr == importer)
	{
		return;
	}
	if (!importer->Initialize(path.c_str(), -1, mFbxManager->GetIOSettings()))
	{
		return;
	}

	FbxScene* scene = FbxScene::Create(mFbxManager, "");
	if (nullptr == scene)
	{
		return;
	}

	importer->Import(scene);
	importer->Destroy();

	// Direct X 좌표계로 변경
	fbxsdk::FbxAxisSystem sceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
	fbxsdk::FbxAxisSystem directXAxisSys(fbxsdk::FbxAxisSystem::eDirectX);
	directXAxisSys.ConvertScene(scene);

	// Quad -> Triangle 변환
	FbxGeometryConverter geometryConverter(mFbxManager);
	geometryConverter.Triangulate(scene, true);

	fbxsdk::FbxNode* rootNode = scene->GetRootNode();
	if (nullptr == rootNode)
	{
		return;
	}

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<Subset> subsets;
	std::vector<Material*> materials;
	std::vector<Bone> bones;
	std::vector<CtrlPoint> controlPoints;

	// Skeleton Bone 계층 구조를 구성한다.
	for (int i = 0; i < rootNode->GetChildCount(); i++)
	{
		fbxsdk::FbxNode* childNode = rootNode->GetChild(i);
		FbxMesh* mesh = (FbxMesh*)childNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = mesh->GetAttributeType();
		if (!mesh || !AttributeType) { continue; }

		switch (AttributeType)
		{
		case fbxsdk::FbxNodeAttribute::eSkeleton:
			ReadBoneHierarchy(scene, childNode, 0, -1, &bones);
			break;
		}
	}

	// FBX 파일에서 Contorl Point, Bone Offset 변환, 정점, 인덱스, 애니메이션 데이터를 처리한다. 
	for (int i = 0; i < rootNode->GetChildCount(); i++)
	{
		fbxsdk::FbxNode* childNode = rootNode->GetChild(i);
		FbxMesh* mesh = (FbxMesh*)childNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = mesh->GetAttributeType();
		if (!mesh || !AttributeType) { continue; }

		if (AttributeType == fbxsdk::FbxNodeAttribute::eMesh)
		{
			ReadControlPoint(scene, childNode, &controlPoints);
			ReadSkeleton(scene, childNode, &bones, &controlPoints);
			ReadMeshData(scene, childNode, mesh, &vertices, &indices, &subsets, &controlPoints);
			CalculateTangent(vertices, indices);
			ReadMaterial(scene, childNode, &materials);

			break;
		}
	}

	MeshFilter* meshFilter = new MeshFilter(vertices, indices);
	_gameObject->AttachComponent(meshFilter);

	MeshRenderer* renderer = nullptr;

	if (bones.size() > 0)
	{
		renderer = new SkinnedMeshRenderer();
		SkinnedMeshRenderer* skinnedMeshRenderer = dynamic_cast<SkinnedMeshRenderer*>(renderer);
		skinnedMeshRenderer->SetBoneHierarchy(bones);
	}
	else
	{
		renderer = new MeshRenderer();
	}

	for (auto mat : materials)
	{
		renderer->AttachMaterial(mat);
	}
	_gameObject->AttachComponent(renderer);
}

void FBXLoader::ReadAnimationFile(std::unordered_map<std::string, AnimationClip>* _animationClips, const std::wstring _filePath)
{
	std::string path;
	path.assign(_filePath.begin(), _filePath.end());

	fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(mFbxManager, "");
	if (nullptr == importer)
	{
		return;
	}
	if (!importer->Initialize(path.c_str(), -1, mFbxManager->GetIOSettings()))
	{
		return;
	}

	FbxScene* scene = FbxScene::Create(mFbxManager, "");
	if (nullptr == scene)
	{
		return;
	}

	importer->Import(scene);
	importer->Destroy();

	// Direct X 좌표계로 변경
	fbxsdk::FbxAxisSystem sceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
	fbxsdk::FbxAxisSystem directXAxisSys(fbxsdk::FbxAxisSystem::eDirectX);
	directXAxisSys.ConvertScene(scene);

	// Quad -> Triangle 변환
	FbxGeometryConverter geometryConverter(mFbxManager);
	geometryConverter.Triangulate(scene, true);

	fbxsdk::FbxNode* rootNode = scene->GetRootNode();
	if (nullptr == rootNode)
	{
		return;
	}

	std::vector<Bone> bones;
	std::vector<CtrlPoint> controlPoints;

	// Skeleton Bone 계층 구조를 구성한다.
	for (int i = 0; i < rootNode->GetChildCount(); i++)
	{
		fbxsdk::FbxNode* childNode = rootNode->GetChild(i);
		FbxMesh* mesh = (FbxMesh*)childNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = mesh->GetAttributeType();
		if (!mesh || !AttributeType) { continue; }

		switch (AttributeType)
		{
		case fbxsdk::FbxNodeAttribute::eSkeleton:
			ReadBoneHierarchy(scene, childNode, 0, -1, &bones);
			break;
		}
	}


	for (int i = 0; i < rootNode->GetChildCount(); i++)
	{
		fbxsdk::FbxNode* childNode = rootNode->GetChild(i);
		FbxMesh* mesh = (FbxMesh*)childNode->GetNodeAttribute();
		fbxsdk::FbxNodeAttribute::EType AttributeType = mesh->GetAttributeType();
		if (!mesh || !AttributeType) { continue; }

		if (AttributeType == fbxsdk::FbxNodeAttribute::eMesh)
		{
			AnimationClip clip;
			ReadAnimationClip(scene, childNode, clip, &bones);
			
			_animationClips->insert({ clip.Name, clip });
			break;
		}
	}
}

void FBXLoader::ReadControlPoint(const fbxsdk::FbxScene * _scene, const fbxsdk::FbxNode * _node, std::vector<CtrlPoint>* _controlPoints)
{
	FbxMesh * mesh = (FbxMesh*)_node->GetNodeAttribute();

	unsigned int ctrlPointsCount = mesh->GetControlPointsCount();
	for (unsigned int i = 0; i < ctrlPointsCount; ++i)
	{
		CtrlPoint cp;

		XMFLOAT3 position;
		position.x = static_cast<float>(mesh->GetControlPointAt(i).mData[0]);
		position.y = static_cast<float>(mesh->GetControlPointAt(i).mData[1]);
		position.z = static_cast<float>(mesh->GetControlPointAt(i).mData[2]);

		cp.Position = position;
		_controlPoints->emplace_back(cp);
	}
}

void FBXLoader::ReadMeshData(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode* _node, const fbxsdk::FbxMesh * _fbxMesh, std::vector<Vertex>* _vertices, std::vector<uint16_t>* _indices, std::vector<Subset>* _subsets, std::vector<CtrlPoint>* _controlPoints)
{
	std::unordered_map<std::string, uint32_t> IndexMapping;
	uint32_t VertexIndex = 0;
	int count = _fbxMesh->GetPolygonCount(); // Triangle

	uint32_t startIndex = _indices->size();

	for (int i = 0; i < count; ++i)
	{
		// Vertex and Index info
		for (int j = 0; j < 3; ++j)
		{
			int controlPointIndex = _fbxMesh->GetPolygonVertex(i, j);
			CtrlPoint ctrlPoint = _controlPoints->at(controlPointIndex);

			// Normal
			FbxVector4 normal;
			_fbxMesh->GetPolygonVertexNormal(i, j, normal);

			// UV
			float* lUVs = NULL;

			FbxStringList lUVNames;
			_fbxMesh->GetUVSetNames(lUVNames);

			const char* lUVName = NULL;
			if (lUVNames.GetCount())
			{
				lUVName = lUVNames[0];
			}

			FbxVector2 pUVs;
			bool bUnMappedUV;
			if (!_fbxMesh->GetPolygonVertexUV(i, j, lUVName, pUVs, bUnMappedUV))
			{
				MessageBox(0, L"UV not found", 0, 0);
			}

			Vertex temp;

			// Position
			temp.Position.x = ctrlPoint.Position.x;
			temp.Position.y = ctrlPoint.Position.y;
			temp.Position.z = ctrlPoint.Position.z;

			// Normal
			temp.Normal.x = static_cast<float>(normal.mData[0]);
			temp.Normal.y = static_cast<float>(normal.mData[1]);
			temp.Normal.z = static_cast<float>(normal.mData[2]);

			// UV
			temp.TexCoord.x = static_cast<float>(pUVs.mData[0]);
			temp.TexCoord.y = 1.0f - static_cast<float>(pUVs.mData[1]);	

			std::string hash = HashFunction(temp);

			auto lookup = IndexMapping.find(hash);
			if (lookup != IndexMapping.end())
			{
				_indices->push_back(lookup->second);
			}
			else
			{
				ctrlPoint.SortBlendingInfoByWeight();
				for (int k = 0; k < 4; k++)
				{
					temp.BoneIndices[k] = ctrlPoint.BoneInfo[k].BoneIndices;
					temp.BoneWeights[k] = ctrlPoint.BoneInfo[k].BoneWeight;
				}

				unsigned int index = _vertices->size();
				IndexMapping[hash] = index;
				_vertices->push_back(temp);
				_indices->push_back(index);

			}
		}
	}

	Subset subset;
	subset.MaterialName = _node->GetName();
	subset.BaseVertexLocation = _vertices->size();
	subset.IndexCount = _indices->size() - startIndex;
	subset.StartIndexLocation = startIndex;
}

void FBXLoader::ReadSkeleton(const fbxsdk::FbxScene* _scene, const fbxsdk::FbxNode * _node, std::vector<Bone>*_bones, std::vector<CtrlPoint>* _controlPoints)
{
	FbxMesh* mesh = (FbxMesh*)_node->GetNodeAttribute();
	FbxAMatrix geometryTransform = GetGeometryTransformation(_node);

	for (int deformerIndex = 0; deformerIndex < mesh->GetDeformerCount(); ++deformerIndex)
	{
		FbxSkin* pCurrSkin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!pCurrSkin) { continue; }

		for (int clusterIndex = 0; clusterIndex < pCurrSkin->GetClusterCount(); ++clusterIndex)
		{
			fbxsdk::FbxCluster* cluster = pCurrSkin->GetCluster(clusterIndex);

			std::string currJointName = cluster->GetLink()->GetName();
			BYTE currJointIndex;
			for (currJointIndex = 0; currJointIndex < _bones->size(); ++currJointIndex)
			{
				if (_bones->at(currJointIndex).Name == currJointName)
					break;
			}
			
			FbxAMatrix transformMatrix, transformLinkMatrix;
			FbxAMatrix globalBindposeInverseMatrix;

			transformMatrix = cluster->GetTransformMatrix(transformMatrix);	// The transformation of the mesh at binding time
			transformLinkMatrix = cluster->GetTransformLinkMatrix(transformLinkMatrix);	// The transformation of the cluster(joint) at binding time from joint space to world space
			globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			// Set the BoneOffset Matrix
			DirectX::XMFLOAT4X4 boneOffset;
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					boneOffset.m[i][j] = globalBindposeInverseMatrix.Get(i, j);
				}
			}
			_bones->at(currJointIndex).OffsetTransform = boneOffset;

			auto controlPointIndices = cluster->GetControlPointIndices();
			for (int i = 0; i < cluster->GetControlPointIndicesCount(); ++i)
			{
				BoneIndexAndWeight currBoneIndexAndWeight;
				currBoneIndexAndWeight.BoneIndices = currJointIndex;
				currBoneIndexAndWeight.BoneWeight = cluster->GetControlPointWeights()[i];

				_controlPoints->at(controlPointIndices[i]).BoneInfo.push_back(currBoneIndexAndWeight);
				_controlPoints->at(controlPointIndices[i]).BoneName = currJointName;
			}
		}
	}

	// 더미 값을 추가한다.
	BoneIndexAndWeight currBoneIndexAndWeight;	
	currBoneIndexAndWeight.BoneIndices= 0;
	currBoneIndexAndWeight.BoneWeight = 0.0f;
	for (UINT i = 0; i < mesh->GetControlPointsCount(); i++)
	{
		for (UINT j = _controlPoints->at(i).BoneInfo.size(); j < 4; j++)
		{
			_controlPoints->at(i).BoneInfo.push_back(currBoneIndexAndWeight);
		}
	}

}

void FBXLoader::ReadBoneHierarchy(const fbxsdk::FbxScene * _scene, const fbxsdk::FbxNode * _node, int _index, int _parent, std::vector<Bone>* _bones)
{
	Bone bone;
	bone.Index = _index;
	bone.ParentIndex = _parent;
	bone.Name = _node->GetName();
	_bones->push_back(bone);

	for (int i = 0; i < _node->GetChildCount(); ++i)
	{
		ReadBoneHierarchy(_scene, _node->GetChild(i), _bones->size(), _index, _bones);
	}
}

void FBXLoader::ReadMaterial(const fbxsdk::FbxScene * _scene, const fbxsdk::FbxNode * _node, std::vector<Material*>* _materials)
{
	int materialCount = _node->GetMaterialCount();

	for (int i = 0; i < materialCount; i++)
	{
		fbxsdk::FbxSurfaceMaterial* fbxMaterial = _node->GetMaterial(i);
		Material *material = new Material(_node->GetName());
		ReadMaterialAttribute(fbxMaterial, material);
		ReadTexture(fbxMaterial, material);

		_materials->emplace_back(material);
	}

}

void FBXLoader::ReadAnimationClip(const fbxsdk::FbxScene * _scene, const fbxsdk::FbxNode * _node, AnimationClip & _animationClip, std::vector<Bone>* _bones)
{
	FbxMesh* mesh = (FbxMesh*)_node->GetNodeAttribute();
	FbxAMatrix geometryTransform = GetGeometryTransformation(_node);

	AnimationClip animation;
	animation.BoneAnimations.resize(_bones->size());

	// Deformer - Cluster - Link
	// Deformer
	for (int deformerIndex = 0; deformerIndex < mesh->GetDeformerCount(); ++deformerIndex)
	{
		FbxSkin* pCurrSkin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!pCurrSkin) 
		{
			continue; 
		}

		// Cluster
		for (int clusterIndex = 0; clusterIndex < pCurrSkin->GetClusterCount(); ++clusterIndex)
		{
			fbxsdk::FbxCluster* cluster = pCurrSkin->GetCluster(clusterIndex);

			// To find the index that matches the name of the current joint
			std::string currJointName = cluster->GetLink()->GetName();
			BYTE currJointIndex; // current joint index

			for (currJointIndex = 0; currJointIndex < _bones->size(); ++currJointIndex)
			{
				if (_bones->at(currJointIndex).Name == currJointName)
					break;
			}

			// Set the Bone Animation Matrix
			BoneAnimation boneAnimation;
			fbxsdk::FbxAnimStack* animStack = _scene->GetSrcObject<fbxsdk::FbxAnimStack>(0);
			fbxsdk::FbxAnimEvaluator* pSceneEvaluator = const_cast<FbxScene*>(_scene)->GetAnimationEvaluator();

			fbxsdk::FbxTakeInfo* takeInfo = _scene->GetTakeInfo(animStack->GetName());
			boneAnimation.Name = currJointName;
			FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
			FbxTime end = takeInfo->mLocalTimeSpan.GetStop();

			// TRqS transformation and Time per frame
			FbxLongLong index;

			for (index = (int)start.GetFrameCount(FbxTime::eFrames24); index <= (int)end.GetFrameCount(FbxTime::eFrames24); ++index)
			{
				FbxTime currTime;
				currTime.SetFrame(index, FbxTime::eFrames24);

				KeyFrame keyFrame;
				keyFrame.TimePosition = index;

				FbxAMatrix currentTransformOffset = pSceneEvaluator->GetNodeGlobalTransform(const_cast<FbxNode*>(_node), currTime) * geometryTransform;
				FbxAMatrix temp = currentTransformOffset.Inverse() * pSceneEvaluator->GetNodeGlobalTransform(cluster->GetLink(), currTime);

				// Transition, Scaling and Rotation Quaternion
				FbxVector4 TS = temp.GetT();
				keyFrame.Position = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])};
				TS = temp.GetS();
				keyFrame.Scale = {
					static_cast<float>(TS.mData[0]),
					static_cast<float>(TS.mData[1]),
					static_cast<float>(TS.mData[2])};
				FbxQuaternion Q = temp.GetQ();
				keyFrame.Quaternion = {
					static_cast<float>(Q.mData[0]),
					static_cast<float>(Q.mData[1]),
					static_cast<float>(Q.mData[2]),
					static_cast<float>(Q.mData[3])};

				boneAnimation.Keyframes.push_back(keyFrame);
			}

			animation.BoneAnimations[currJointIndex] = boneAnimation;
		}
	}

	BoneAnimation InitBoneAnim;

	// Initialize InitBoneAnim
	for (int i = 0; i < _bones->size(); ++i)
	{
		int KeyframeSize = animation.BoneAnimations[i].Keyframes.size();
		if (KeyframeSize != 0)
		{
			for (int j = 0; j < KeyframeSize; ++j)
			{
				KeyFrame key;

				key.TimePosition = j / 24.0f;
				InitBoneAnim.Keyframes.push_back(key);
			}
			break;
		}
	}

	for (int i = 0; i < _bones->size(); ++i)
	{
		if (animation.BoneAnimations[i].Keyframes.size() != 0)
		{
			continue;
		}

		animation.BoneAnimations[i] = InitBoneAnim;
	}

	if (animation.BoneAnimations.size() > 0)
	{
		animation.FrameCount = animation.BoneAnimations[0].Keyframes.size();
		animation.Duration = animation.BoneAnimations[0].Keyframes.back().TimePosition / 24.0f;
		animation.FrameRate = 1.0f / 24.0f;
	}

	_animationClip = animation;
}

void FBXLoader::CalculateTangent(std::vector<Vertex>& _vertices, std::vector<uint16_t>& _indices)
{
	for (UINT i = 0; i < _indices.size(); i += 3)
	{
		Vertex& v0 = _vertices[_indices[i]];
		Vertex& v1 = _vertices[_indices[i + 1]];
		Vertex& v2 = _vertices[_indices[i + 2]];

		XMFLOAT3 e0 = XMFLOAT3(v1.Position.x - v0.Position.x, v1.Position.y - v0.Position.y, v1.Position.z - v0.Position.z);
		XMFLOAT3 e1 = XMFLOAT3(v2.Position.x - v0.Position.x, v2.Position.y - v0.Position.y, v2.Position.z - v0.Position.z);

		XMFLOAT2 u0 = XMFLOAT2(v0.TexCoord.x - v0.TexCoord.x, v1.TexCoord.y - v0.TexCoord.y);
		XMFLOAT2 u1 = XMFLOAT2(v2.TexCoord.x - v0.TexCoord.x, v2.TexCoord.y - v0.TexCoord.y);

		XMFLOAT3 tangent = XMFLOAT3((u0.x * e0.x + u0.y * e1.x), (u0.x * e0.y + u0.y * e1.y), (u0.x * e0.z + u0.y * e1.z));

		tangent.x /= tangent.x + tangent.y + tangent.z;
		tangent.y /= tangent.x + tangent.y + tangent.z;
		tangent.z /= tangent.x + tangent.y + tangent.z;

		auto isZero = [](XMFLOAT3 _float3) {return _float3.x == 0 && _float3.y == 0 && _float3.z == 0; };
		auto average = [](XMFLOAT3 _one, XMFLOAT3 _two) { return XMFLOAT3((_one.x + _two.x) * 0.5f, (_one.y + _two.y) * 0.5f, (_one.z + _two.z) * 0.5f); };

		v0.TangentU = isZero(v0.TangentU) ? tangent : average(v0.TangentU, tangent);
		v1.TangentU = isZero(v1.TangentU) ? tangent : average(v1.TangentU, tangent);
		v2.TangentU = isZero(v2.TangentU) ? tangent : average(v2.TangentU, tangent);
	}
}

void FBXLoader::ReadMaterialAttribute(fbxsdk::FbxSurfaceMaterial* _fbxMaterial, Material* _mat)
{
	fbxsdk::FbxDouble3 double3;
	fbxsdk::FbxDouble double1;
	if (_fbxMaterial->GetClassId().Is(fbxsdk::FbxSurfacePhong::ClassId))
	{
		// Diffuse Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Diffuse;
		XMFLOAT4 diffuse = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetAlbedo(diffuse);

		// Amibent Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Ambient;
		XMFLOAT4 ambient = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetAmbient(ambient);

		// Specular Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Specular;
		XMFLOAT4 spcular = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetSpecular(spcular);

		// Roughness 
		double1 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Shininess;
		_mat->SetShineness(static_cast<float>(double1));
	}
	else if (_fbxMaterial->GetClassId().Is(fbxsdk::FbxSurfaceLambert::ClassId))
	{
		// Diffuse Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Diffuse;
		XMFLOAT4 diffuse = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetAlbedo(diffuse);

		// Amibent Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Ambient;
		XMFLOAT4 ambient = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetAmbient(ambient);

		// Specular Color
		double3 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Specular;
		XMFLOAT4 spcular = XMFLOAT4(static_cast<float>(double3.mData[0]),
			static_cast<float>(double3.mData[1]),
			static_cast<float>(double3.mData[2]),
			1.0f);
		_mat->SetSpecular(diffuse);

		// Roughness 
		double1 = reinterpret_cast<FbxSurfacePhong *>(_fbxMaterial)->Shininess;
		_mat->SetShineness(static_cast<float>(double1));
	}
}

void FBXLoader::ReadTexture(fbxsdk::FbxSurfaceMaterial * _fbxMaterial, Material * _mat)
{
	unsigned int textureIndex = 0;
	fbxsdk::FbxProperty property;

	FBXSDK_FOR_EACH_TEXTURE(textureIndex)
	{
		property = _fbxMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[textureIndex]);
		if (property.IsValid())
		{
			unsigned int textureCount = property.GetSrcObjectCount<FbxTexture>();
			for (unsigned int i = 0; i < textureCount; ++i)
			{
				FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(i);
				if (layeredTexture)
				{
					throw std::exception("Layered Texture is currently unsupported\n");
				}
				else
				{
					FbxTexture* texture = property.GetSrcObject<FbxTexture>(i);
					if (texture)
					{
						std::string textureType = property.GetNameAsCStr();
						FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);

						if (fileTexture)
						{
							// 사용자 계정명이 영어로 지어진 경우에, fileTexture->GetFileName()을 바로 쓰면 됨.
							std::string path = fileTexture->GetFileName();
							path = "FBX/" + GetFilePathUsingAbsolutePath(path);

							std::wstring wPath;
							wPath.assign(path.begin(), path.end());

							if (textureType == "DiffuseColor")
							{
								Texture* diffuseMap = new Texture("diffuse", wPath);
								diffuseMap->CreateTextureFromWICFileFormat();
								_mat->AttachDiffuseMap(diffuseMap);
							}
							else if (textureType == "SpecularColor")
							{
								Texture* specularMap = new Texture("specular", wPath);
								specularMap->CreateTextureFromWICFileFormat();
								_mat->AttachSpecularMap(specularMap);
							}
							else if (textureType == "NormalMap")
							{
								Texture* normalMap = new Texture("normal", wPath);
								normalMap->CreateTextureFromWICFileFormat();
								_mat->AttachNormalMap(normalMap);
							}
						}
					}
				}
			}
		}
	}
}

fbxsdk::FbxAMatrix FBXLoader::GetGeometryTransformation(const FbxNode * _node)
{
	const FbxVector4 lT = _node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = _node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = _node->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

XMFLOAT2 FBXLoader::ReadUV(const FbxMesh * _mesh, int _controlPointIndex, int _indexCounter)
{
	if (_mesh->GetElementUVCount() < 1)
		return XMFLOAT2(0, 0);

	const FbxGeometryElementUV* vertexUv = _mesh->GetElementUV(0);
	XMFLOAT2 result;

	switch (vertexUv->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexUv->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexUv->GetDirectArray().GetAt(_controlPointIndex).mData[0]);
			result.y = static_cast<float>(vertexUv->GetDirectArray().GetAt(_controlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexUv->GetIndexArray().GetAt(_controlPointIndex);
			result.x = static_cast<float>(vertexUv->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexUv->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		default:
			return XMFLOAT2{ 0,0 };
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexUv->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		case FbxGeometryElement::eIndexToDirect:
		{
			result.x = static_cast<float>(vertexUv->GetDirectArray().GetAt(_indexCounter).mData[0]);
			result.y = static_cast<float>(vertexUv->GetDirectArray().GetAt(_indexCounter).mData[1]);
		}
		break;
		default:
			return XMFLOAT2{ 0,0 };
		}
		break;
	}

	return result;
}

XMFLOAT3 FBXLoader::ReadTangent(const FbxMesh * _mesh, int _controlPointIndex, int _vertexCounter)
{
	if (_mesh->GetElementTangentCount() < 1)
		return XMFLOAT3();

	const FbxGeometryElementTangent* vertexTangent = _mesh->GetElementTangent(0);
	XMFLOAT3 result = XMFLOAT3();

	switch (vertexTangent->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_controlPointIndex).mData[0]);
			result.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_controlPointIndex).mData[1]);
			result.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_controlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(_controlPointIndex);
			result.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_vertexCounter).mData[0]);
			result.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_vertexCounter).mData[1]);
			result.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(_vertexCounter).mData[2]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(_vertexCounter);
			result.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		}
	default:
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	}

	return result;
}

XMFLOAT3 FBXLoader::ReadBinormal(const FbxMesh * _mesh, int _controlPointIndex, int _vertexCounter)
{
	if (_mesh->GetElementBinormalCount() < 1)
		return XMFLOAT3();

	const FbxGeometryElementBinormal* vertexNormal = _mesh->GetElementBinormal(0);
	XMFLOAT3 result;

	switch (vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(_controlPointIndex);
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		default:
			return XMFLOAT3{ 0,0,0 };
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[2]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(_vertexCounter);
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		default:
			return XMFLOAT3{ 0,0,0 };
		}
		break;
	}

	return result;
}

XMFLOAT3 FBXLoader::ReadNormal(const FbxMesh * _mesh, int _controlPointIndex, int _vertexCounter)
{
	if (_mesh->GetElementNormalCount() < 1)
		return XMFLOAT3();

	const FbxGeometryElementNormal* vertexNormal = _mesh->GetElementNormal(0);
	XMFLOAT3 result;

	switch (vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_controlPointIndex).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(_controlPointIndex);
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		default:
			return XMFLOAT3{ 0,0,0 };
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(_vertexCounter).mData[2]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(_vertexCounter);
			result.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			result.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;
		default:
			return XMFLOAT3{ 0,0,0 };
		}
		break;
	}

	return result;
}

std::string FBXLoader::HashFunction(Vertex& _vertex)
{
	std::string data = std::to_string(_vertex.Position.x) + std::to_string(_vertex.Position.y) + std::to_string(_vertex.Position.z)
		+ std::to_string(_vertex.Normal.x) + std::to_string(_vertex.Normal.y) + std::to_string(_vertex.Normal.z)
		+ std::to_string(_vertex.TexCoord.x) + std::to_string(_vertex.TexCoord.x)
		+ std::to_string(_vertex.TangentU.x) + std::to_string(_vertex.TangentU.y) + std::to_string(_vertex.TangentU.z);
	
	return data;
}