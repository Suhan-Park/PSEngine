#include "MeshFilter.h"
#include "Vertex.h"

MeshFilter::MeshFilter()
{

}

MeshFilter::MeshFilter(std::string _path)
{

}

MeshFilter::MeshFilter(std::vector<Vertex> _vertices, std::vector<uint16_t> _indices) : mVertices(_vertices), mIndices(_indices)
{

}

MeshFilter::MeshFilter(std::string _name, std::vector<Vertex> _vertices, std::vector<uint16_t> _indices) : mName(_name), mVertices(_vertices), mIndices(_indices)
{
}


MeshFilter::~MeshFilter()
{

}

void MeshFilter::Awake()
{
	mVertexByteStride = sizeof(Vertex);
	mVertexBufferByteSize = static_cast<UINT>(mVertices.size() * sizeof(Vertex));

	mIndexBufferByteSize = static_cast<UINT>(mIndices.size() * sizeof(uint16_t));
	mIndexCount = static_cast<UINT>(mIndices.size());
}

void MeshFilter::Update(const float _deltaTime)
{
}

void MeshFilter::FixedUpdate(const float _fixedDltaTime)
{
}

void MeshFilter::Draw(const float _deltaTime)
{
}

void MeshFilter::Destroy()
{
	mVertices.clear();
	mVertexBufferByteSize = 0;
	std::vector<Vertex>().swap(mVertices);

	mIndices.clear();
	mIndexBufferByteSize = 0;
	mIndexCount = 0;
	std::vector<uint16_t>().swap(mIndices);
}