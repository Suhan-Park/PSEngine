#ifndef _LIGHT_SYSTEM_H_
#define _LIGHT_SYSTEM_H_

const int MaxDirectionalLightsCount = 3;
const int MaxPointLightsCount = 6;
const int MaxSpotLightsCount = 6;

#include "Light.h"

class LightSystem final
{
	friend class D3DApplication;

private:
	
	LightSystem();
	~LightSystem() = default;

	LightSystem(const LightSystem& _rhs) = delete;
	LightSystem& operator = (const LightSystem& _rhs) = delete;
	LightSystem(LightSystem&& _rhs) = delete;
	LightSystem& operator = (LightSystem&& _rhs) = delete;

public:

	static LightSystem* Instance();

public:

	inline Light* GetMainDirectionalLight()
	{
		if (!mLights[LightType::Directional].empty())
		{
			return mLights[LightType::Directional][0];
		}

		return nullptr;
	}

public:

	void AddLight(Light* _light, LightType _type);
	void RemoveLight(Light* _light, LightType _type);

	void GetLightData(LightData _lightData[], int _size);
	int GetDirectionalLightCount();
	int GetPointLightCount();
	int GetSpotLightCount();

private:

	std::unordered_map<LightType, std::vector<Light*>> mLights;
	std::unordered_map<LightType, int> mLimitCount;

	int mDirectionalLightsCount = 0;
	int mPointLightsCount = 0;
	int mSpotLightsCount = 0;

private:

	void Initialize();
	void Release();
};

#endif