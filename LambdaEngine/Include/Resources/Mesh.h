#pragma once
#include "LambdaEngine.h"

#include "Math/Math.h"

#include "Rendering/Core/API/IBuffer.h"

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

		/*
		* Calculate the tangent of this vertex given the other vertices, v1 and v2, in the triangle containing this vertex		
		*/
		void CalculateTangent(const Vertex& v1, const Vertex& v2)
		{
			glm::vec3 edge1 = v1.Position - this->Position;
			glm::vec3 edge2 = v2.Position - this->Position;
			glm::vec2 deltaUV1 = v1.TexCoord - this->TexCoord;
			glm::vec2 deltaUV2 = v2.TexCoord - this->TexCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			this->Tangent = tangent;
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
