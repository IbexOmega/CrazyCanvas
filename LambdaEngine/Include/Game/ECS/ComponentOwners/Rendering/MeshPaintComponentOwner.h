#pragma once

#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	class DeviceChild;

	class MeshPaintComponentOwner : public ComponentOwner
	{
	public:
		DECL_SINGLETON_CLASS(MeshPaintComponentOwner);

		bool Init();
		bool Release();

		void Tick(uint64 modFrameIndex);

	private:
		void ReleaseMeshPaintComponent(MeshPaintComponent& meshPaintComponent);

	public:
		FORCEINLINE static MeshPaintComponentOwner* GetInstance() { return &s_Instance; }

	private:
		static void MeshPaintDestructor(MeshPaintComponent& meshPaintComponent, Entity entity);

	private:
		static MeshPaintComponentOwner s_Instance;
		TArray<DeviceChild*> m_ResourcesToRelease[BACK_BUFFER_COUNT];
		uint64 m_ModFrameIndex = 0;
	};
}
