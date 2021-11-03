#ifndef _MESH_FILTER_H_
#define _MESH_FILTER_H_

#include "Component.h"
#include "Vertex.h"

class MeshFilter : public Component
{
public:

	MeshFilter();
	MeshFilter(std::string _path);
	MeshFilter(std::vector<Vertex> _vertices, std::vector<uint16_t> _indices);
	MeshFilter(std::string _name ,std::vector<Vertex> _vertices, std::vector<uint16_t> _indices);
	virtual ~MeshFilter();

private:

	MeshFilter(const MeshFilter& _rhs) = delete;
	MeshFilter& operator = (const MeshFilter& _rhs) = delete;
	MeshFilter(MeshFilter&& _rhs) = delete;
	MeshFilter& operator = (MeshFilter&& _rhs) = delete;
	
public:

	virtual void Awake() override;
	virtual void Update(const float _deltaTime) override;
	virtual void FixedUpdate(const float _fixedDltaTime) override;
	virtual void Draw(const float _deltaTime) override;
	virtual void Destroy() override;

	inline const std::vector<Vertex>& Vertices() const { return mVertices; }
	inline const std::vector<uint16_t>& Indices() const { return mIndices; }
	
	const UINT VertexBufferByteSize() const { return mVertexBufferByteSize; }
	const UINT IndexBufferByteSize() const { return mIndexBufferByteSize; }
	const UINT VertexByteStride() const { return mVertexByteStride; }
	const UINT IndexCount() const { return mIndexCount; }

	DXGI_FORMAT IndexFormat() { return mIndexFormat; }

private:

	std::string mName = "None";
	std::vector<Vertex> mVertices;
	std::vector<uint16_t> mIndices;

	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	UINT mIndexBufferByteSize = 0;
	UINT mIndexCount = 0;

	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R16_UINT;
};

#endif