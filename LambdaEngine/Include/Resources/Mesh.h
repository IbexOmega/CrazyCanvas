#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

#include "Physics/BoundingBox.h"

#include "Containers/PrehashedString.h"

#include "Rendering/Core/API/Buffer.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4324) //Disable alignment warning
#endif

#define MAX_PRIMS 124
#define MAX_VERTS 64

#define INVALID_JOINT_ID 0xff

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

	// Meshlet data
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

	using MeshIndexType = uint32; // Moved out from mesh due to dependency issue
	using JointIndexType = uint8; // Limited to 128 bones, enough for now

	// Animation
	struct VertexJointData
	{
		struct JointData
		{
			// TODO: Use less space
			int32	JointID = INVALID_JOINT_ID;
			float32	Weight	= 0.0f;
		};

		JointData Joint0;
		JointData Joint1;
		JointData Joint2;
		JointData Joint3;
	};

	struct VertexWeight
	{
		MeshIndexType	VertexIndex;
		float32			VertexWeight;
	};

	struct Joint
	{
		// TODO: Change into mat4x3
		glm::mat4x4		InvBindTransform;
		PrehashedString	Name;
		JointIndexType	ParentBoneIndex = INVALID_JOINT_ID;
	};

	struct Skeleton
	{
		using JointHashTable = THashTable<PrehashedString, JointIndexType, PrehashedStringHasher>;

		glm::mat4x4		InverseGlobalTransform;
		TArray<Joint>	Joints;
		JointHashTable	JointMap;
	};

	struct SkeletonPose
	{
		Skeleton*			pSkeleton;
		TArray<glm::mat4>	LocalTransforms;
		TArray<glm::mat4>	GlobalTransforms;
	};

	struct Animation
	{
		struct Channel
		{
			struct KeyFrame
			{
				glm::vec3	Value;
				float64		Time;
			};

			struct RotationKeyFrame
			{
				glm::quat	Value;
				float64		Time;
			};

			PrehashedString				Name;
			TArray<KeyFrame>			Positions;
			TArray<KeyFrame>			Scales;
			TArray<RotationKeyFrame>	Rotations;
		};

		inline float64 DurationInSeconds() const
		{
			return DurationInTicks / TicksPerSecond;
		}

		PrehashedString	Name;
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
		TArray<VertexJointData>	VertexJointData;
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
