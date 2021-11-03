#include "Scene.h"

#include "D3DApplication.h"

#include "GameObject.h"
#include "Transform.h"
#include "AudioSource.h"
#include "Light.h"
#include "Camera.h"
#include "CameraController.h"
#include "ModelImporter.h"
#include "FBXLoader.h"

Scene::Scene()
{

}

Scene::~Scene()
{
	
}

void Scene::AddGameObject(GameObject* _gameObject)
{
	mGameObjects.emplace_back(_gameObject, [] (GameObject* _obj) {
		_obj->_DESTORY_();
	});
}

void Scene::Awake()
{
	GameObject* light = GameObject::Instantiate();
	light->AttachComponent(new Light());
	light->GetComponent<Transform>()->SetWorldRotation(XMFLOAT3(35.0f, 210.0f, 10.0f));
	light->GetComponent<Light>()->SetDiffuse(XMFLOAT4(0.88f, 0.83f, 0.78f, 1.0f));
	light->GetComponent<Light>()->SetIntensity(0.69f);

	GameObject* camera = GameObject::Instantiate();
	INT width = D3DApplication::Instance()->ApplicationWindowWidth();
	INT height = D3DApplication::Instance()->ApplicationWindowHeight();
	camera->AttachComponent(new Camera(static_cast<float>(width), static_cast<float>(height)));
	camera->AttachComponent(new CameraController());
	camera->GetComponent<Transform>()->Translate(XMFLOAT3(0.0f, -2.0f, 20.0f));

	AudioSource* audio = new AudioSource();
	audio->CreateSound("Sounds/Morning Kiss.mp3");
	camera->AttachComponent(audio);
	audio->Play();

	ModelImporter::Instance()->Initialize();
	FBXLoader::Instance()->Initialize();
}

void Scene::Update(const float _deltaTime)
{

}

void Scene::Draw(const float _deltaTime)
{

}

void Scene::Destroy()
{

}

void Scene::_AWAKE_()
{
	Awake();
}

void Scene::_UPDATE_(const float _deltaTime)
{
	Update(_deltaTime);

	for (auto& iter : mGameObjects)
	{
		std::weak_ptr<GameObject> wp = iter;
		std::shared_ptr<GameObject> gameObject = wp.lock();
		gameObject.get()->_UPDATE_(_deltaTime);
	}
}

void Scene::_FIXED_UPDATE_(const float _deltaTime)
{
	for (auto& iter : mGameObjects)
	{
		std::weak_ptr<GameObject> wp = iter;
		std::shared_ptr<GameObject> gameObject = wp.lock();
		gameObject.get()->_FIXED_UPDATE_(_deltaTime);
	}
}

void Scene::_DRAW_(const float _deltaTime)
{
	Draw(_deltaTime);

	for (auto& iter : mGameObjects)
	{
		std::weak_ptr<GameObject> wp = iter;
		std::shared_ptr<GameObject> gameObject = wp.lock();
		gameObject.get()->_DRAW_(_deltaTime);
	}
}

void Scene::_DESTORY_()
{
	for (auto& iter : mGameObjects)
	{
		std::weak_ptr<GameObject> wp = iter;
		std::shared_ptr<GameObject> gameObject = wp.lock();
		gameObject.get()->_DESTORY_();
	}

	Destroy();
	mGameObjects.clear();
}