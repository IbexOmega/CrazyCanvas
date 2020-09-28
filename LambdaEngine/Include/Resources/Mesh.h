#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

#include "Rendering/Core/API/Buffer.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4324) //Disable alignment warning
#endif

#define MAX_PRIMS 124
#define MAX_VERTS 64

namespace LambdaEngine
{
	struct Vertex
	{
		alignas(16) glm::vec3 Position;
		alignas(16) glm::vec3 Normal;
		alignas(16) glm::vec3 Tangent;
		alignas(16) glm::vec2 TexCoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Normal == other.Normal && Tangent == other.Tangent && TexCoord == other.TexCoord;
		}
	};

	struct Meshlet
	{
		uint32 VertCount;
		uint32 VertOffset;
		uint32 PrimCount;
		uint32 PrimOffset;
	};

	struct PackedTriangle
	{
		uint8 i0;
		uint8 i1;
		uint8 i2;
		uint8 Padding;
	};

	struct Skeleton
	{
	};

	struct Mesh
	{
		using IndexType = uint32;

		inline ~Mesh()
		{
			SAFEDELETE(pSkeleton);
		}

		TArray<Vertex>			Vertices;
		TArray<IndexType>		Indices;
		TArray<IndexType>		UniqueIndices;
		TArray<PackedTriangle>	PrimitiveIndices;
		TArray<Meshlet>			Meshlets;
		Skeleton*				pSkeleton = nullptr;
	};

	class MeshFactory
	{
	public:
		static Mesh* CreateQuad();
		static void GenerateMeshlets(Mesh* pMesh, uint32 maxVerts = MAX_VERTS, uint32 maxPrims = MAX_PRIMS);
	};
}

namespace std
{
	template<> struct hash<LambdaEngine::Vertex>
	{
		size_t operator()(LambdaEngine::Vertex const& vertex) const
		{
			return
				((hash<glm::vec3>()(vertex.Position) ^
				 (hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^
				 (hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}
