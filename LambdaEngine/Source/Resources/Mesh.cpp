#include "Resources/Mesh.h"

namespace LambdaEngine
{
	Mesh* MeshFactory::CreateQuad()
	{
		Vertex vertices[4] = 
		{
			{ glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
			{ glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
			{ glm::vec3( 1.0f, 0.0f,  1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
			{ glm::vec3( 1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f) }
		};

		uint32 indices[6] =
		{
			2, 0, 1,
			3, 0, 2
		};

		Vertex* pVertexArray = DBG_NEW Vertex[ARR_SIZE(vertices)];
		memcpy(pVertexArray, vertices, sizeof(Vertex) * ARR_SIZE(vertices));

		uint32* pIndexArray = DBG_NEW uint32[ARR_SIZE(indices)];
		memcpy(pIndexArray, indices, sizeof(uint32) * ARR_SIZE(indices));

		Mesh* pMesh = DBG_NEW Mesh();
		pMesh->pVertexArray = pVertexArray;
		pMesh->pIndexArray	= pIndexArray;
		pMesh->VertexCount	= ARR_SIZE(vertices);
		pMesh->IndexCount	= ARR_SIZE(indices);

		return pMesh;
	}
}
