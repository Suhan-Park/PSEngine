# PSEngine
#### DirectX 12 Based Engine Framework (C++ Based)
#### Implemented Based on CBD(Component Based Development)
#### [[Video 1]](https://youtu.be/6k3URDt5IMU)   [[Video 2]](https://youtu.be/jgJ7R8a4eA8)
![Figure](https://user-images.githubusercontent.com/93682690/140207131-0314c2bf-5d2c-4db7-9cfd-03f2497dc27f.png)
***
### 클래스 구조도

### 주요 구현 
* Scene Structure
* Based on Component Based Development

```
	GameObject* sphere = GameObject::Instantiate();
	sphere->SetName("Sphere");
	sphere->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 1.5f, 0.0f));

	MeshFilter* meshfilter = PrimitiveGeometry::Sphere();
	sphere->AttachComponent(meshfilter);

	MeshRenderer* meshrender = new MeshRenderer();
	sphere->AttachComponent(meshrender);

	RigidBody* rigidBody = new RigidBody();
	rigidBody->SetRigidBodyType(RIGIDBODY_TYPE::DYNAMIC);
	sphere->AttachComponent(rigidBody);
	BoxCollider* boxCollider = new BoxCollider();
	sphere->AttachComponent(boxCollider);
```
* Mesh Skinning
* Lighting & Shading System - Directional Light Only
* Post Processing - SSAO(Screen Space Ambient Occlusion)
* Model Importer - Using FBX SDK, assimp SDK
* Sound - FMOD
* Collision System - PhysX SDK 
* Supported File Format

File|File Format
|---|---|
|3D Model|fbx, dae, 3ds, ... (Formats supported by assimp sdk)
|Texture|dds, png
|Sound|wav, mp3, ... (Formats supported by assimp fmod)

### Requirements
* FMOD
* FBX SDK 2020.0
* assimp-5.0.1
* PhysX-4.1
* DirectXTK

***

### 향후 계획
* Lighting, Post Processing
* Refactoring
* Tessellation
* PBR
