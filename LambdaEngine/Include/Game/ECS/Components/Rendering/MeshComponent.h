#pragma once

namespace LambdaEngine
{
	struct MeshComponent
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};

	struct StaticMeshComponent : MeshComponent
	{
	};

	struct DynamicMeshComponent : MeshComponent
	{
	};
}

