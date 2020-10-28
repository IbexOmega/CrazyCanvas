#pragma once

#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

namespace LambdaEngine
{
	class MeshPaintComponentOwner : ComponentOwner
	{
	public:
		DECL_SINGLETON_CLASS(MeshPaintComponentOwner);

		bool Init();

	private:
		static void MeshPaintDestructor(MeshPaintComponent& meshPaintComponent, Entity entity);

	public:
		FORCEINLINE static MeshPaintComponentOwner* GetInstance() { return &s_Instance; }

	private:
		static MeshPaintComponentOwner s_Instance;
	};
}
