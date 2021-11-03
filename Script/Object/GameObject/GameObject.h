#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_

class Component;
class Scene;

class GameObject final : public Object
{
	friend class Scene;
	
public:
	GameObject() = default;
private:
	virtual ~GameObject() = default; //�����Լ��� ���弼��.
private:

	GameObject(const GameObject& _rhs) = delete;
	GameObject& operator = (const GameObject& _rhs) = delete;
	GameObject(GameObject&& _rhs) = delete;
	GameObject& operator = (GameObject&& _rhs) = delete;

public:

	static GameObject* Instantiate();

	template <typename T>
	T* const GetComponent()
	{
		const auto& type = typeid(T);
		
		for (auto iter = mComponents.begin(); iter != mComponents.end(); iter++)
		{
			// �ݺ����̹Ƿ� *iter�� �ؾ���.

			if (typeid(*iter->get()) == type)
			{
				return static_cast<T*>(iter->get());
			}
		}

		return nullptr;
	}

	void AttachComponent(Component* _component);
	void DetachComponent(Component* _component);

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

	void OnCollisionEnter(GameObject* _gameObject);
	void OnCollisionStay(GameObject* _gameObject);
	void OnCollisionExit(GameObject* _gameObject);

	void OnTriggerEnter(GameObject* _gameObject);
	void OnTriggerStay(GameObject* _gameObject);
	void OnTriggerExit(GameObject* _gameObject);

private:

	std::list<std::shared_ptr<Component>> mComponents;

	// �ý��� ȣ�� �Լ� (����ڿ��� ���� �ȵ�)
private:

	virtual void _AWAKE_() final;
	virtual void _UPDATE_(const float _deltaTime) final;
	virtual void _FIXED_UPDATE_(const float _deltaTime) final;
	virtual void _DRAW_(const float _deltaTime) final;
	virtual void _DESTORY_() final;
};

#endif