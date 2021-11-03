#pragma once

struct KeyFrame
{
	KeyFrame() = default;
	~KeyFrame() = default;

	std::string Name;
	FLOAT    TimePosition;
	XMFLOAT3 Position;
	XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMFLOAT4 Quaternion = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
};