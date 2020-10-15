#pragma once

#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

namespace LambdaEngine
{
	class MeshPaintComponentOwner : public ComponentOwner
	{
	public:
		DECL_SINGLETON_CLASS(MeshPaintComponentOwner);

		bool Init();

	private:
		void MeshPaintDestructor(MeshPaintComponent& meshPaintComponent);

	public:
		FORCEINLINE static MeshPaintComponentOwner* GetInstance() { return &s_Instance; }

	private:
		static MeshPaintComponentOwner s_Instance;
	};
}
