#include "LightSystem.h"

#include "Transform.h"

LightSystem::LightSystem()
{
	mLimitCount[LightType::Directional] = MaxDirectionalLightsCount;
	mLimitCount[LightType::Point] = MaxPointLightsCount;
	mLimitCount[LightType::Spot] = MaxSpotLightsCount;

}

LightSystem* LightSystem::Instance()
{
	static LightSystem lightSystem;
	return &lightSystem;
}

void LightSystem::AddLight(Light* _light, LightType _type)
{
	if (mLights[_type].size() > mLimitCount[_type])
	{
		return;
	}

	mLights[_type].emplace_back(_light);
}

void LightSystem::RemoveLight(Light * _light, LightType _type)
{
	mLights[_type].erase(std::remove(mLights[_type].begin(), mLights[_type].end(), _light), mLights[_type].end());
}

void LightSystem::GetLightData(LightData _lightData[], int _size)
{
	int arraySize = 0;
	for (auto maxCount : mLimitCount)
	{
		arraySize += mLimitCount[maxCount.first];
	}

	if (_size != arraySize)
	{
		return;
	}

	for (int i = 0; i < mLights[LightType::Directional].size(); i++)
	{
		_lightData[i].Position = mLights[LightType::Directional][i]->GetComponent<Transform>()->GetWorldPosition();
		XMStoreFloat3(&_lightData[i].Direction, mLights[LightType::Directional][i]->GetComponent<Transform>()->GetForwardVector());
		_lightData[i].Diffuse = mLights[LightType::Directional][i]->GetDiffuse();
		_lightData[i].Range = mLights[LightType::Directional][i]->GetRange();
		_lightData[i].Intensity = mLights[LightType::Directional][i]->GetIntensity();
		_lightData[i].SpotAngle = mLights[LightType::Directional][i]->GetSpotAngle();
	}

	for (int i = MaxDirectionalLightsCount, j = 0; i < MaxDirectionalLightsCount + mLights[LightType::Point].size(); i++, j++)
	{
		_lightData[i].Position = mLights[LightType::Point][j]->GetComponent<Transform>()->GetWorldPosition();
		XMStoreFloat3(&_lightData[i].Direction, mLights[LightType::Point][j]->GetComponent<Transform>()->GetForwardVector());
		_lightData[i].Diffuse = mLights[LightType::Point][j]->GetDiffuse();
		_lightData[i].Range = mLights[LightType::Point][j]->GetRange();
		_lightData[i].Intensity = mLights[LightType::Point][j]->GetIntensity();
		_lightData[i].SpotAngle = mLights[LightType::Point][j]->GetSpotAngle();
	}

	for (int i = MaxDirectionalLightsCount + MaxPointLightsCount, j = 0; i < MaxDirectionalLightsCount + MaxPointLightsCount + mLights[LightType::Spot].size(); i++, j++)
	{
		_lightData[i].Position = mLights[LightType::Spot][j]->GetComponent<Transform>()->GetWorldPosition();
		XMStoreFloat3(&_lightData[i].Direction, mLights[LightType::Spot][j]->GetComponent<Transform>()->GetForwardVector());
		_lightData[i].Diffuse = mLights[LightType::Spot][j]->GetDiffuse();
		_lightData[i].Range = mLights[LightType::Spot][j]->GetRange();
		_lightData[i].Intensity = mLights[LightType::Spot][j]->GetIntensity();
		_lightData[i].SpotAngle = mLights[LightType::Spot][j]->GetSpotAngle();
	}
}

int LightSystem::GetDirectionalLightCount()
{
	return static_cast<int>(mLights[LightType::Directional].size());
}

int LightSystem::GetPointLightCount()
{
	return static_cast<int>(mLights[LightType::Point].size());
}

int LightSystem::GetSpotLightCount()
{
	return static_cast<int>(mLights[LightType::Spot].size());
}

void LightSystem::Initialize()
{
}

void LightSystem::Release()
{
	mLights.clear();
}