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
	GameObject* Sphere = GameObject::Instantiate();
	Sphere->SetName("Sphere");
	Sphere->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 1.5f, 0.0f));

	MeshFilter* Meshfilter = PrimitiveGeometry::Sphere();
	Sphere->AttachComponent(Meshfilter);

	MeshRenderer* MeshRenderer = new MeshRenderer();
	Sphere->AttachComponent(MeshRenderer);

	RigidBody* RigidBody = new RigidBody();
	RigidBody->SetRigidBodyType(RIGIDBODY_TYPE::DYNAMIC);
	Sphere->AttachComponent(RigidBody);
	BoxCollider* BoxCollider = new BoxCollider();
	Sphere->AttachComponent(BoxCollider);
```
* Mesh Skinning
* Lighting & Shading System - Directional Light Only
* Post Processing - SSAO(Screen Space Ambient Occlusion)
* Model Importer - Using FBX SDK, ASSIMP SDK
* Sound - FMOD
* Collision System - PhysX SDK 
* File Format
머리1 | 머리2 | 머리3 | 뚝배기
---- | ---- | ---- | ----
다리 | 다리1 | 다리2 | 뚝배기깹니다
금 | 의 | 환 | 향
### 클래스 구조도

### 사용 라이브러리 / SDK
#### - FBX SDK
#### - ASSIMP
#### - FMOD
#### - PhysX-4.1
#### - DirectXTK
