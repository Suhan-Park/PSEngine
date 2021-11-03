#ifndef _SCENE_H_
#define _SCENE_H_

class GameObject;

class Scene abstract : public Object
{
	friend class D3DApplication;

public:

	Scene();
	~Scene();

private:

	Scene(const Scene& _rhs) = delete;
	Scene& operator = (const Scene& _rhs) = delete;
	Scene(Scene&& _rhs) = delete;
	Scene& operator = (Scene&& _rhs) = delete;

public:
 
	void AddGameObject(GameObject* _gameObject);

protected:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

private:

	std::list<std::shared_ptr<GameObject>> mGameObjects;

	// 시스템 호출 함수 (사용자에게 노출 안됨)
private:
	virtual void _AWAKE_() final;
	virtual void _UPDATE_(const float _deltaTime) final;
	virtual void _FIXED_UPDATE_(const float _deltaTime) final;
	virtual void _DRAW_(const float _deltaTime) final;
	virtual void _DESTORY_() final;

};

#endif // !_SCENE_H_
