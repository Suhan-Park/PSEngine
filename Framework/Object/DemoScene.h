#ifndef _DEMO_SCENE_H_
#define _DEMO_SCENE_H_

#include "Scene.h"

class GameObject;

class DemoScene final : public Scene 
{
public:

	DemoScene();
	~DemoScene();

private:

	DemoScene(const DemoScene & _rhs) = delete;
	DemoScene & operator = (const DemoScene & _rhs) = delete;
	DemoScene(DemoScene && _rhs) = delete;
	DemoScene & operator = (DemoScene && _rhs) = delete;

protected:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;
};

#endif // !_DEMO_SCENE_H_