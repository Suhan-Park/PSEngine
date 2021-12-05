# PSEngine
#### DirectX 12 Based Engine Framework (C++ Based)
#### Implemented Based on CBD(Component Based Development)
#### [[Video 1]](https://youtu.be/MINmL6p0SGc)   [[Video 2]](https://youtu.be/jgJ7R8a4eA8)
![Figure](https://user-images.githubusercontent.com/93682690/140207131-0314c2bf-5d2c-4db7-9cfd-03f2497dc27f.png)
***
### 클래스 구조도

![Untitled Diagram-Page-1 drawio (1)](https://user-images.githubusercontent.com/93682690/144744902-c9e73512-97a7-41d2-ab45-693f23f4a843.png)

![Untitled Diagram-Page-2 drawio](https://user-images.githubusercontent.com/93682690/144719774-86810c9b-b42e-4a6b-94bb-02e4aa143ea5.png)

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
|Texture|dds, png, ... (Formats supported by assimp sdk)
|Sound|wav, mp3, ... (Formats supported by assimp fmod)

### Requirements
* FMOD
* FBX SDK 2020.0
* assimp-5.0.1
* PhysX-4.1
* DirectXTK

***

### 향후 계획
* Deferred Rendering (Lighting, Post Processing)
* Refactoring
* Tessellation
* PBR
