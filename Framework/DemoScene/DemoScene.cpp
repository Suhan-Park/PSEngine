#include "DemoScene.h"

#include "GameObject.h"
#include "Transform.h"
#include "Input.h"
#include "MeshFilter.h"
#include "MeshRenderer.h"
#include "SkinnedMeshRender.h"
#include "Texture.h"
#include "Material.h"
#include "AnimationClip.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "RigidBody.h"
#include "PrimitiveGeometry.h"
#include "ModelImporter.h"
#include "FBXLoader.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CameraController.h"

DemoScene::DemoScene()
{
}

DemoScene::~DemoScene()
{
}

void DemoScene::Awake()
{
	Scene::Awake();

#pragma region Floor

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

#pragma endregion

#pragma region Ceiling

	GameObject* ceiling = GameObject::Instantiate();
	ceiling->SetName("Ceiling");
	ceiling->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 10.0f, 0.0f));
	ceiling->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(20.0f, 1.0f, 20.0f));

	MeshFilter* ceilingMeshfilter = PrimitiveGeometry::Box();
	ceiling->AttachComponent(ceilingMeshfilter);

	MeshRenderer* ceilingMeshrender = new MeshRenderer();
	ceiling->AttachComponent(ceilingMeshrender);

	Texture* ceilingDiffuseMap = new Texture("wall", L"Textures/Ceiling/plaster common diffuse.dds");
	Texture* ceilingNormalMap = new Texture("wall", L"Textures/Ceiling/plaster common normal.dds");
	ceilingDiffuseMap->Initialize();
	ceilingNormalMap->Initialize();

	Material* ceilingMaterial = new Material();
	ceilingMaterial->SetShineness(0.8f);
	ceilingMaterial->SetFresnelR0(XMFLOAT3(0.95f, 0.93f, 0.88f));
	ceilingMaterial->SetTiling(XMFLOAT2(1.0f, 1.0f));
	ceilingMaterial->SetAlbedo(XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f));
	ceilingMaterial->AttachDiffuseMap(ceilingDiffuseMap);
	ceilingMaterial->AttachNormalMap(ceilingNormalMap);
	ceilingMeshrender->AttachMaterial(ceilingMaterial);

#pragma endregion

#pragma region Walls

	GameObject* wall1 = GameObject::Instantiate();
	wall1->SetName("Wall");
	wall1->GetComponent<Transform>()->Translate(XMFLOAT3(-10.0f, 5.0f, 0.0f));
	wall1->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.5f, 10.0f, 20.0f));
	MeshFilter* wallMeshfilter = PrimitiveGeometry::Box();
	wall1->AttachComponent(wallMeshfilter);

	MeshRenderer* wall1Meshrender = new MeshRenderer();
	wall1->AttachComponent(wall1Meshrender);

	Texture* wallDiffuseMap = new Texture("wall", L"Textures/Wall/WhitePlaster.dds");
	wallDiffuseMap->Initialize();

	Material* wallMaterial = new Material();
	wallMaterial->SetShineness(0.8f);
	wallMaterial->SetFresnelR0(XMFLOAT3(0.95f, 0.93f, 0.88f));
	wallMaterial->SetTiling(XMFLOAT2(4.0f, 4.0f));
	wallMaterial->AttachDiffuseMap(wallDiffuseMap);
	wall1Meshrender->AttachMaterial(wallMaterial);

	GameObject* wall2 = GameObject::Instantiate();
	wall2->SetName("Wall");
	wall2->GetComponent<Transform>()->Translate(XMFLOAT3(10.0f, 5.0f, 0.0f));
	wall2->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.5f, 10.0f, 20.0f));

	wall2->AttachComponent(wallMeshfilter);

	MeshRenderer* wall2Meshrender = new MeshRenderer();
	wall2->AttachComponent(wall2Meshrender);
	wall2Meshrender->AttachMaterial(wallMaterial);

#pragma endregion

#pragma region Window

	Material* frameMaterial = new Material();
	frameMaterial->SetShineness(0.8f);
	frameMaterial->SetFresnelR0(XMFLOAT3(0.15f, 0.33f, 0.48f));
	frameMaterial->SetTiling(XMFLOAT2(1.0f, 1.0f));
	frameMaterial->SetAlbedo(XMFLOAT4(0.6f, 0.58f, 0.5f, 1.0f));

#pragma region Frame 1

	GameObject* frame1 = GameObject::Instantiate();
	frame1->SetName("Frame");
	frame1->GetComponent<Transform>()->Translate(XMFLOAT3(-3.3f, 5.0f, 10.0f));
	frame1->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.2f, 10.0f, 0.1f));

	MeshFilter* frameMeshfilter = PrimitiveGeometry::Box();
	frame1->AttachComponent(frameMeshfilter);

	MeshRenderer* frameMeshrender1 = new MeshRenderer();
	frame1->AttachComponent(frameMeshrender1);

	frameMeshrender1->AttachMaterial(frameMaterial);

#pragma endregion

#pragma region Frame 2

	GameObject* frame2 = GameObject::Instantiate();
	frame2->SetName("Frame");
	frame2->GetComponent<Transform>()->Translate(XMFLOAT3(3.3f, 5.0f, 10.0f));
	frame2->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.2f, 10.0f, 0.1f));

	frame2->AttachComponent(frameMeshfilter);

	MeshRenderer* frameMeshrender2 = new MeshRenderer();
	frame2->AttachComponent(frameMeshrender2);

	frameMeshrender2->AttachMaterial(frameMaterial);

#pragma endregion

#pragma region Frame 3

	GameObject* frame3 = GameObject::Instantiate();
	frame3->SetName("Frame");
	frame3->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 0.0f, 10.0f));
	frame3->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(20.0f, 0.5f, 0.1f));

	frame3->AttachComponent(frameMeshfilter);

	MeshRenderer* frameMeshrender3 = new MeshRenderer();
	frame3->AttachComponent(frameMeshrender3);

	frameMeshrender3->AttachMaterial(frameMaterial);

#pragma endregion


#pragma endregion

#pragma region Curtain

	GameObject* curtain = GameObject::Instantiate();
	curtain->SetName("Curtain");
	ModelImporter::Instance()->ReadFile(curtain, L"Models/Curtains_5x.fbx", L"Curtains_5x.fbx");

	Texture* curtainDiffuseMap = new Texture("curtain", L"Textures/Curtain/Curtains_House_BaseColor.dds");
	Texture* curtainNormalMap = new Texture("curtain", L"Textures/Curtain/Curtains_House_Normal.dds");

	curtainDiffuseMap->Initialize();
	curtainNormalMap->Initialize();

	Material* curtainMaterial = new Material();
	curtainMaterial->AttachDiffuseMap(curtainDiffuseMap);
	curtainMaterial->AttachNormalMap(curtainNormalMap);
	curtainMaterial->SetAlbedo(XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f));

	curtain->GetComponent<MeshRenderer>()->AttachMaterial(curtainMaterial);

	curtain->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.0425f, 0.04f, 0.04f));
	curtain->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(26.25f, 8.65f, 9.5f));
	curtain->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 0.0f, 0.0f));

#pragma endregion

#pragma region Chandelier

	GameObject* chandelier = GameObject::Instantiate();
	chandelier->SetName("Chandelier");
	ModelImporter::Instance()->ReadFile(chandelier, L"Models/Lamp_chandelier.fbx", L"Lamp_chandelier.fbx");

	Texture* chandelierDiffuseMap = new Texture("curtain", L"Textures/Chandelier/Lamps_House_BaseColor.dds");
	Texture* chandelierNormalMap = new Texture("curtain", L"Textures/Chandelier/Lamps_House_Normal.dds");

	chandelierDiffuseMap->Initialize();
	chandelierNormalMap->Initialize();

	Material* chandelierMaterial = new Material();
	chandelierMaterial->AttachDiffuseMap(chandelierDiffuseMap);
	chandelierMaterial->AttachNormalMap(chandelierNormalMap);
	chandelierMaterial->SetAlbedo(XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f));

	chandelier->GetComponent<MeshRenderer>()->AttachMaterial(chandelierMaterial);

	chandelier->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.0425f, 0.04f, 0.04f));
	chandelier->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(0.0f, 9.5f, 4.5f));
	chandelier->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 0.0f, 0.0f));
#pragma endregion

#pragma region Bed

	GameObject* bed = GameObject::Instantiate();
	bed->SetName("Bed");
	ModelImporter::Instance()->ReadFile(bed, L"Models/Bed_queen.fbx", L"Bed_queen.fbx");

	Texture* bedDiffuseMap = new Texture("curtain", L"Textures/Bed/Bed_BaseColor.dds");
	Texture* bedNormalMap = new Texture("curtain", L"Textures/Bed/Bed_Normal.dds");

	bedDiffuseMap->Initialize();
	bedNormalMap->Initialize();

	Material* bedMaterial = new Material();
	bedMaterial->AttachDiffuseMap(bedDiffuseMap);
	bedMaterial->AttachNormalMap(bedNormalMap);
	bedMaterial->SetShineness(0.8f);

	bed->GetComponent<MeshRenderer>()->AttachMaterial(bedMaterial);

	bed->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.035f, 0.035f, 0.035f));
	bed->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(6.0f, 0.0f, 5.5f));
	bed->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 90.0f, 0.0f));

#pragma endregion

#pragma region Table

	GameObject* table = GameObject::Instantiate();
	table->SetName("Table");
	ModelImporter::Instance()->ReadFile(table, L"Models/Kitchen_table_small.fbx", L"Kitchen_table_small.fbx");

	Texture* tableDiffuseMap = new Texture("table", L"Textures/Table/Kitchen_TableChair_BaseColor.dds");
	Texture* tableNormalMap = new Texture("table", L"Textures/Table/Kitchen_TableChair_Normal.dds");

	tableDiffuseMap->Initialize();
	tableNormalMap->Initialize();

	Material* tableMaterial = new Material();
	tableMaterial->AttachDiffuseMap(tableDiffuseMap);
	tableMaterial->AttachNormalMap(tableNormalMap);
	tableMaterial->SetShineness(0.8f);
	tableMaterial->SetAlbedo(XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f));

	table->GetComponent<MeshRenderer>()->AttachMaterial(tableMaterial);

	table->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.025f, 0.025f, 0.025f));
	table->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(-4.5f, 0.0f, 4.0f));
	table->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 90.0f, 0.0f));

#pragma endregion

#pragma region Couch

	GameObject* couch = GameObject::Instantiate();
	couch->SetName("Couch");
	ModelImporter::Instance()->ReadFile(couch, L"Models/Couch_2seat.fbx", L"Couch_2seat.fbx");

	Texture* couchDiffuseMap = new Texture("table", L"Textures/Couch/Couch_Fabric_BaseColor.dds");
	Texture* couchNormalMap = new Texture("table", L"Textures/Couch/Couch_Fabric_Normal.dds");

	couchDiffuseMap->Initialize();
	couchNormalMap->Initialize();

	Material* couchMaterial = new Material();
	couchMaterial->AttachDiffuseMap(couchDiffuseMap);
	couchMaterial->AttachNormalMap(couchNormalMap);
	couchMaterial->SetShineness(0.5f);
	couchMaterial->SetAlbedo(XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f));

	couch->GetComponent<MeshRenderer>()->AttachMaterial(couchMaterial);

	couch->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	couch->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(-8.5f, 0.0f, 4.7f));
	couch->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 270.0f, 0.0f));

	//
	GameObject* couch2 = GameObject::Instantiate();
	couch2->SetName("Couch2");
	ModelImporter::Instance()->ReadFile(couch2, L"Models/Couch_1seat.fbx", L"Couch_1seat.fbx");

	couch2->GetComponent<MeshRenderer>()->AttachMaterial(couchMaterial);

	couch2->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	couch2->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(-5.5f, 0.0f, 8.3f));
	couch2->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 330.0f, 0.0f));

#pragma endregion

#pragma region BookShelf

	GameObject* shelf = GameObject::Instantiate();
	shelf->SetName("shelf");
	ModelImporter::Instance()->ReadFile(shelf, L"Models/Bookshelf.fbx", L"Bookshelf.fbx");

	Texture* shelfDiffuseMap = new Texture("shelf", L"Textures/BookShelf/Fireplace_BaseColor.dds");
	Texture* shelfNormalMap = new Texture("shelf", L"Textures/BookShelf/Fireplace_Normal.dds");

	shelfDiffuseMap->Initialize();
	shelfNormalMap->Initialize();

	Material* shelfMaterial = new Material();
	shelfMaterial->AttachDiffuseMap(shelfDiffuseMap);
	shelfMaterial->AttachNormalMap(shelfNormalMap);
	shelfMaterial->SetShineness(0.8f);
	shelfMaterial->SetAlbedo(XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f));

	shelf->GetComponent<MeshRenderer>()->AttachMaterial(shelfMaterial);

	GameObject* books = GameObject::Instantiate();
	books->SetName("books");
	ModelImporter::Instance()->ReadFile(books, L"Models/Book_set_bookshelf_A.fbx", L"Book_set_bookshelf_A.fbx");

	Texture* bookDiffuseMap = new Texture("table", L"Textures/BookShelf/Books_BaseColor.dds");
	Texture* bookNormalMap = new Texture("table", L"Textures/BookShelf/Books_Normal.dds");

	bookDiffuseMap->Initialize();
	bookNormalMap->Initialize();

	Material* bookMaterial = new Material();
	bookMaterial->AttachDiffuseMap(bookDiffuseMap);
	bookMaterial->AttachNormalMap(bookNormalMap);
	bookMaterial->SetShineness(0.8f);
	bookMaterial->SetAlbedo(XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f));

	books->GetComponent<MeshRenderer>()->AttachMaterial(bookMaterial);

	books->GetComponent<Transform>()->SetParent(shelf->GetComponent<Transform>());

	shelf->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	shelf->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(9.5f, 0.0f, -6.0f));
	shelf->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 90.0f, 0.0f));

	GameObject* shelf2 = GameObject::Instantiate();
	shelf2->SetName("shelf");
	ModelImporter::Instance()->ReadFile(shelf2, L"Models/Bookshelf.fbx", L"Bookshelf.fbx");

	shelf2->GetComponent<MeshRenderer>()->AttachMaterial(shelfMaterial);

	GameObject* books2 = GameObject::Instantiate();
	books2->SetName("books");
	ModelImporter::Instance()->ReadFile(books2, L"Models/Book_set_bookshelf_A.fbx", L"Book_set_bookshelf_A.fbx");

	books2->GetComponent<MeshRenderer>()->AttachMaterial(bookMaterial);

	books2->GetComponent<Transform>()->SetParent(shelf2->GetComponent<Transform>());

	shelf2->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	shelf2->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(9.5f, 0.0f, -1.0f));
	shelf2->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 90.0f, 0.0f));

#pragma endregion

#pragma region Carpet

	GameObject* carpet = GameObject::Instantiate();
	carpet->SetName("Carpet");
	ModelImporter::Instance()->ReadFile(carpet, L"Models/Carpet_living_room.fbx", L"Carpet_living_room.fbx");

	Texture* carpetDiffuseMap = new Texture("Carpet", L"Textures/Carpet/Carpet_A_BaseColor.dds");
	Texture* carpetNormalMap = new Texture("Carpet", L"Textures/Carpet/Carpet_A_Normal.dds");

	carpetDiffuseMap->Initialize();
	carpetNormalMap->Initialize();

	Material* carpetMaterial = new Material();
	carpetMaterial->AttachDiffuseMap(carpetDiffuseMap);
	carpetMaterial->AttachNormalMap(carpetNormalMap);
	carpetMaterial->SetShineness(0.8f);
	carpetMaterial->SetAlbedo(XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f));

	carpet->GetComponent<MeshRenderer>()->AttachMaterial(carpetMaterial);

	carpet->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	carpet->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, 0.0f, 0.0f));
	carpet->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(3.0f, 0.05f, -3.0f));

#pragma endregion

#pragma region Painting

	GameObject* painting = GameObject::Instantiate();
	painting->SetName("Painting");
	ModelImporter::Instance()->ReadFile(painting, L"Models/Painting_H.fbx", L"Painting_H.fbx");

	Texture* paintingDiffuseMap = new Texture("Painting", L"Textures/Painting/Paintings_interior_BaseColor.dds");
	Texture* paintingNormalMap = new Texture("Painting", L"Textures/Painting/Paintings_interior_Normal.dds");

	paintingDiffuseMap->Initialize();
	paintingNormalMap->Initialize();

	Material* paintingMaterial = new Material();
	paintingMaterial->AttachDiffuseMap(paintingDiffuseMap);
	paintingMaterial->AttachNormalMap(paintingNormalMap);
	paintingMaterial->SetShineness(0.8f);
	paintingMaterial->SetAlbedo(XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f));

	painting->GetComponent<MeshRenderer>()->AttachMaterial(paintingMaterial);

	painting->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.04f, 0.04f, 0.04f));
	painting->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(90.0f, -90.0f, 0.0f));
	painting->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(-9.8f, 6.5f, 6.0f));

#pragma endregion

#pragma region Cabinet

	GameObject* cabinet = GameObject::Instantiate();
	cabinet->SetName("Cabinet");
	ModelImporter::Instance()->ReadFile(cabinet, L"Models/Kitcabinet_full_cabinet.fbx", L"Kitcabinet_full_cabinet.fbx");

	Texture* cabinetDiffuseMap = new Texture("Cabinet", L"Textures/Cabinet/Kitchen_Cabinet_Wood_BaseColor.dds");
	Texture* cabinetNormalMap = new Texture("Cabinet", L"Textures/Cabinet/Kitchen_Cabinet_Wood_Normal.dds");

	cabinetDiffuseMap->Initialize();
	cabinetNormalMap->Initialize();

	Material* cabinetMaterial = new Material();
	cabinetMaterial->AttachDiffuseMap(cabinetDiffuseMap);
	cabinetMaterial->AttachNormalMap(cabinetNormalMap);
	cabinetMaterial->SetShineness(0.8f);
	cabinetMaterial->SetAlbedo(XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f));

	cabinet->GetComponent<MeshRenderer>()->AttachMaterial(cabinetMaterial);

	cabinet->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	cabinet->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(0.0f, 0.0f, 90.0f));
	cabinet->GetComponent<Transform>()->SetWorldPosition(XMFLOAT3(-9.0f, 2.0f, -10.5f));

#pragma endregion

#pragma region Character

	GameObject* go = GameObject::Instantiate();
	FBXLoader::Instance()->ReadFile(go, L"FBX/Mannequin.fbx");
	std::unordered_map<std::string, AnimationClip> animations;
	FBXLoader::Instance()->ReadAnimationFile(&animations, L"FBX/Idle.fbx");
	go->GetComponent<SkinnedMeshRenderer>()->SetAnimationClips(animations);
	go->GetComponent<SkinnedMeshRenderer>()->SetAnimationClip("None");

	go->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(0.03f, 0.03f, 0.03f));
	go->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(0.00f, 0.0f, 0.00f));
	go->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 6.0f, 7.5f));

#pragma endregion

#pragma region PhysX Test
	/*
	GameObject* sphere = GameObject::Instantiate();
	sphere->SetName("Sphere");
	sphere->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, 1.5f, 0.0f));
	sphere->AttachComponent(new CameraController());

	MeshFilter* meshfilter = PrimitiveGeometry::Sphere();
	sphere->AttachComponent(meshfilter);

	MeshRenderer* meshrender = new MeshRenderer();
	sphere->AttachComponent(meshrender);

	RigidBody* rigidBody = new RigidBody();
	rigidBody->SetRigidBodyType(RIGIDBODY_TYPE::DYNAMIC);
	sphere->AttachComponent(rigidBody);
	SphereCollider* sphereCollider = new SphereCollider();
	sphere->AttachComponent(sphereCollider);

	GameObject* box = GameObject::Instantiate();
	box->SetName("Box");
	box->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, -6.0f, 0.0f));
	box->GetComponent<Transform>()->SetWorldScale(XMFLOAT3(10.0f, 1.0f, 10.0f));

	RigidBody* rigidBody1 = new RigidBody();
	rigidBody1->SetRigidBodyType(RIGIDBODY_TYPE::STATIC);
	box->AttachComponent(rigidBody1);
	BoxCollider* boxCollider1 = new BoxCollider();
	boxCollider1->SetBoxSize(XMFLOAT3(10.0f, 1.0f, 10.0f));
	box->AttachComponent(boxCollider1);

	MeshFilter* meshfilter1 = PrimitiveGeometry::Box();
	box->AttachComponent(meshfilter1);

	MeshRenderer* meshrender1 = new MeshRenderer();
	box->AttachComponent(meshrender1);

	Texture* diffuseMap = new Texture("br", L"Textures/bricks.dds");
	Texture* normalMap = new Texture("brn", L"Textures/bricks_nmap.dds");
	Texture* diffuseMap1 = new Texture("tile", L"Textures/checkboard.dds");
	diffuseMap->Initialize();
	normalMap->Initialize();
	diffuseMap1->Initialize();

	Material* material = new Material();
	material->SetShineness(1.0f);
	material->SetFresnelR0(XMFLOAT3(0.95f, 0.93f, 0.88f));
	meshrender->AttachMaterial(material);
	material->AttachDiffuseMap(diffuseMap);
	material->AttachNormalMap(normalMap);

	Material* material1 = new Material();
	material1->SetShineness(0.6f);
	material1->SetTiling(XMFLOAT2(5.0f, 5.0f));
	material1->SetFresnelR0(XMFLOAT3(0.15f, 0.13f, 0.38f));
	meshrender1->AttachMaterial(material1);
	material1->AttachDiffuseMap(diffuseMap1);
	*/
#pragma endregion
}

void DemoScene::Update(const float _deltaTime)
{
	Scene::Update(_deltaTime);
}

void DemoScene::Draw(const float _deltaTime)
{
	Scene::Draw(_deltaTime);
}

void DemoScene::Destroy()
{
	Scene::Destroy();
}
