#ifndef _ANIMATOR_H_
#define _ANIMATOR_H_

#include "Component.h"
#include "AnimationClip.h"

class Animator : public Component
{
public:
	
	Animator() = default;
	virtual ~Animator() = default;

private:

	Animator(const Animator& _rhs) = delete;
	Animator& operator = (const Animator& _rhs) = delete;
	Animator(Animator&& _rhs) = delete;
	Animator& operator = (Animator&& _rhs) = delete;

public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

private:

	std::unordered_map<std::string, AnimationClip> mAnimations;

};

#endif