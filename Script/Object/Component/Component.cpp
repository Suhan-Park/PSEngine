#include "Component.h"
#include "GameObject.h"

Component::~Component()
{
	mGameObject = nullptr;
}

void Component::OnCollisionEnter(GameObject* _gameObject)
{
}

void Component::OnCollisionStay(GameObject* _gameObject)
{
}

void Component::OnCollisionExit(GameObject* _gameObject)
{
}

void Component::OnTriggerEnter(GameObject* _gameObject)
{
}

void Component::OnTriggerStay(GameObject* _gameObject)
{
}

void Component::OnTriggerExit(GameObject* _gameObject)
{
}
