#ifndef _VERTEX_H_
#define _VERTEX_H_

struct Vertex
{
	Vertex() {}
	Vertex(
		float _px, float _py, float _pz,
		float _nx, float _ny, float _nz,
		float _tx, float _ty, float _tz,
		float _u,  float _v) :
		Position(_px, _py, _pz),
		Normal(_nx, _ny, _nz),
		TangentU(_tx, _ty, _tz),
		TexCoord(_u, _v) {}

	Vertex(
		const DirectX::XMFLOAT3& _p,
		const DirectX::XMFLOAT3& _n,
		const DirectX::XMFLOAT3& _t,
		const DirectX::XMFLOAT2& _uv) :
		Position(_p),
		Normal(_n),
		TangentU(_t),
		TexCoord(_uv) {}

    DirectX::XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT2 TexCoord = XMFLOAT2(0.0f, 0.0f);
	DirectX::XMFLOAT3 TangentU = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FLOAT BoneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f};
	INT BoneIndices[4] = { 0,0,0,0 };
};

#endif

