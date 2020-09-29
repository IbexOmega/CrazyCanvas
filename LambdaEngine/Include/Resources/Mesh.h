#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"
#include "Physics/BoundingBox.h"

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

	struct VertexBoneData
	{
		struct BoneData
		{
			int32	BoneID = -1;
			float32	Weight = 0.0f;
		};

		BoneData Bone0;
		BoneData Bone1;
		BoneData Bone2;
		BoneData Bone3;
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

	// Moved out from mesh due to dependency issue
	using MeshIndexType = uint32;
	
	struct Skeleton
	{
		struct Bone
		{
			struct Weight
			{
				MeshIndexType	VertexIndex;
				float32			VertexWeight;
			};

			String			Name;
			glm::mat4		OffsetTransform;
			TArray<Weight>	Weights;
		};

		TArray<Bone> Bones;
	};

	struct Animation
	{
		struct Channel
		{
			struct KeyFrame
			{
				glm::vec3	Value;
				float32		Time;
			};

			struct RotationKeyFrame
			{
				glm::quat	Value;
				float32		Time;
			};

			String						Name;
			TArray<KeyFrame>			Positions;
			TArray<KeyFrame>			Scales;
			TArray<RotationKeyFrame>	Rotations;
		};

		String			Name;
		float64			DurationInTicks;
		float64			TicksPerSecond;
		TArray<Channel>	Channels;
	};

	struct Mesh
	{
		inline ~Mesh()
		{
			SAFEDELETE(pSkeleton);
		}

		TArray<Vertex>			Vertices;
		TArray<VertexBoneData>	VertexBoneData;
		TArray<MeshIndexType>	Indices;
		TArray<MeshIndexType>	UniqueIndices;
		TArray<PackedTriangle>	PrimitiveIndices;
		TArray<Meshlet>			Meshlets;
		Skeleton*				pSkeleton = nullptr;
		BoundingBox 			BoundingBox;
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
