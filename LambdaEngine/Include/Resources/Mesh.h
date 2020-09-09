#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

#include "Rendering/Core/API/Buffer.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4324) //Disable alignment warning
#endif

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

	struct Mesh
	{
		~Mesh()
		{
			SAFEDELETE_ARRAY(pVertexArray);
			SAFEDELETE_ARRAY(pIndexArray);
		}

		Vertex* pVertexArray	= nullptr;
		uint32* pIndexArray		= nullptr;
		uint32  VertexCount		= 0;
		uint32  IndexCount		= 0;
	};

	class MeshFactory
	{
	public:
		static Mesh* CreateQuad();
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
