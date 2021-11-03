# PSEngine
#### - DirectX 12 Based Engine Framework (C++ Based)
#### - Implemented Based on CBD(Component Based Development)
#### - [[Video 1]](https://youtu.be/6k3URDt5IMU)   [[Video 2]](https://youtu.be/jgJ7R8a4eA8)
![Figure](https://user-images.githubusercontent.com/93682690/140207131-0314c2bf-5d2c-4db7-9cfd-03f2497dc27f.png)
***
### 주요 구현 
* Scene Structure
* Based On Component Based Development

```
	GameObject* floor = GameObject::Instantiate();
	floor->SetName("Floor");
	floor->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 0.0f, 0.0f));
	floor->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(20.0f, 0.1f, 20.0f));

	RigidBody* floorRigidBody = new RigidBody();
	floor->AttachComponent(floorRigidBody);
	BoxCollider* floorCollider = new BoxCollider();
	floorCollider->SetBoxSize(XMFLOAT3(20.0f, 0.01f, 20.0f));
	floor->AttachComponent(floorCollider);

	MeshFilter* floorMeshfilter = PrimitiveGeometry::Box();
	floor->AttachComponent(floorMeshfilter);

	MeshRenderer* floorMeshrender = new MeshRenderer();
	floor->AttachComponent(floorMeshrender);

	Texture* floorDiffuseMap = new Texture("floor", L"Textures/Floor/T_Herringbone _Red_Albedo.dds");
	Texture* floorNormalMap = new Texture("floor_n", L"Textures/Floor/T_Herringbone_Normal.dds");
	floorDiffuseMap->Initialize();
	floorNormalMap->Initialize();

	Material* floorMaterial = new Material();
	floorMaterial->SetShineness(0.1f);
	floorMaterial->SetTiling(XMFLOAT2(4.0f, 4.0f));
	floorMaterial->SetFresnelR0(XMFLOAT3(0.95f, 0.93f, 0.88f));
	floorMaterial->AttachDiffuseMap(floorDiffuseMap);
	floorMaterial->AttachNormalMap(floorNormalMap);
	floorMeshrender->AttachMaterial(floorMaterial);
```
* Mesh Skinning
* Light System - Directional Light Only
* Post Processing - SSAO(Screen Space Ambient Occlusion)
* Mesh Importer - Using FBX SDK, ASSIMP Library
* Sound - FMOD
* Collision System - PhysX SDK 

### 클래스 구조도

### 사용 라이브러리 / SDK
#### - FBX SDK
#### - ASSIMP
#### - FMOD
#### - PhysX-4.1
#### - DirectXTK
