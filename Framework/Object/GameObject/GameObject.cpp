#include "D3DApplication.h"
#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "Scene.h"

GameObject* GameObject::Instantiate()
{
	GameObject* gameObject = new GameObject();
	D3DApplication::Instance()->GetScene()->AddGameObject(gameObject);

	// GameObject는 Scene에 배치될 객체이기 때문에, Transform 컴포넌트를 반드시 가진다.
	Transform* transform = new Transform();
	gameObject->AttachComponent(transform);
	gameObject->_AWAKE_();

	return gameObject;
}

void GameObject::AttachComponent(Component* _component)
{
	// 기본 컴포넌트들이 중복되지 않도록 해야함.
	_component->SetOwner(this);
	mComponents.emplace_back(_component);
	_component->Awake();
}

void GameObject::DetachComponent(Component* _component)
{
	// Transform 컴포넌트는 지워지지 않아야 한다
	if ("Transform" == typeid(_component).name())
	{
		return;
	}

	if ("Light" == typeid(_component).name())
	{
		return;
	}
	
	const auto type = typeid(_component).name();

	for (auto iter = mComponents.begin(); iter != mComponents.end(); iter++)
	{
		if (typeid(*iter->get()).name() == type)
		{
			iter->get()->Destroy();
			mComponents.erase(iter);
			return;
		}
	}
}

void GameObject::_AWAKE_()
{
	Awake();
}

void GameObject::_UPDATE_(const float _deltaTime)
{
	Update(_deltaTime);
}

void GameObject::_FIXED_UPDATE_(const float _deltaTime)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->FixedUpdate(_deltaTime);
	}
}

void GameObject::_DRAW_(const float _deltaTime)
{
	Draw(_deltaTime);
}

void GameObject::_DESTORY_()
{
	Destroy();
}

void GameObject::Awake()
{

}

void GameObject::Update(const float _deltaTime)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->Update(_deltaTime);
	}
}

void GameObject::Draw(const float _deltaTime)
{

	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->Draw(_deltaTime);
	}
}

void GameObject::Destroy()
{
	mComponents.clear();
	this->~GameObject();
}

void GameObject::OnCollisionEnter(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnCollisionEnter(_gameObject);
	}
}

void GameObject::OnCollisionStay(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnCollisionStay(_gameObject);
	}
}

void GameObject::OnCollisionExit(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnCollisionExit(_gameObject);
	}
}

void GameObject::OnTriggerEnter(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnTriggerEnter(_gameObject);
	}
}

void GameObject::OnTriggerStay(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnTriggerStay(_gameObject);
	}
}

void GameObject::OnTriggerExit(GameObject * _gameObject)
{
	for (auto& iter : mComponents)
	{
		std::weak_ptr<Component> wp = iter;
		std::shared_ptr<Component> component = wp.lock();
		component.get()->OnTriggerExit(_gameObject);
	}
}
