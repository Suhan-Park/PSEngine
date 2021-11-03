#ifndef _OBJECT_H_
#define _OBJECT_H_

class Object abstract
{
public:

	std::string Name()
	{
		return mName;
	}

	void SetName(std::string _name)
	{
		mName = _name;
	}

	// 유저에 의해 재정의 될 수 있는 함수
protected:
	
	virtual void Awake() abstract;
	virtual void Update(const float _deltaTime) abstract;
	virtual void Draw(const float _deltaTime) abstract;
	virtual void Destroy() abstract;

	std::string mName = "";

	// 시스템 호출 함수 (사용자에게 노출 안됨)
private:

	virtual void _AWAKE_() abstract;
	virtual void _UPDATE_(const float _deltaTime) abstract;
	virtual void _DRAW_(const float _deltaTime) abstract;
	virtual void _DESTORY_() abstract;
};

#endif
