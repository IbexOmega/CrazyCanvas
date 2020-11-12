#pragma once

#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	class DeviceChild;

	class MeshPaintComponentOwner : ComponentOwner
	{
	public:
		DECL_SINGLETON_CLASS(MeshPaintComponentOwner);

		bool Init();

	private:
		static void MeshPaintDestructor(MeshPaintComponent& meshPaintComponent, Entity entity);

	public:
		FORCEINLINE static MeshPaintComponentOwner* GetInstance() { return &s_Instance; }

		static void Tick(uint64 modFrameIndex);

	private:
		static MeshPaintComponentOwner s_Instance;
		inline static TArray<DeviceChild*> s_ResourcesToRemove[BACK_BUFFER_COUNT];
		inline static uint64 s_ModFrameIndex = 0;
	};
}
