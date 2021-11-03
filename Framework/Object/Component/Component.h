#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include "GameObject.h"

class Component abstract
{
public:

	Component() = default;
	virtual ~Component();

private:

	Component(const Component& _rhs) = delete;
	Component& operator =(const Component& _rhs) = delete;
	Component(Component&& _rhs) = delete;
	Component& operator =(Component&& _rhs) = delete;

public:

	virtual void Awake() abstract;
	virtual void Update(const float _deltaTime) abstract;
	virtual void FixedUpdate(const float _fixedDltaTime) abstract;
	virtual void Draw(const float _deltaTime) abstract;
	virtual void Destroy() abstract;

	virtual void OnCollisionEnter(GameObject* _gameObject);
	virtual void OnCollisionStay(GameObject* _gameObject);
	virtual void OnCollisionExit(GameObject* _gameObject);

	virtual void OnTriggerEnter(GameObject* _gameObject);
	virtual void OnTriggerStay(GameObject* _gameObject);
	virtual void OnTriggerExit(GameObject* _gameObject);

public:

	inline void SetOwner(GameObject* _gameObject)
	{
		mGameObject = _gameObject;
	}

	inline GameObject* GetOwner()
	{
		return mGameObject;
	}

	template <typename T>
	T* GetComponent()
	{
		return mGameObject->GetComponent<T>();
	}

protected:

	GameObject* mGameObject = nullptr;

};
#endif // !_COMPONENT_H_
