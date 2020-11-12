#include "Game/ECS/ComponentOwners/Rendering/MeshPaintComponentOwner.h"

#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"

namespace LambdaEngine
{
	MeshPaintComponentOwner MeshPaintComponentOwner::s_Instance;

	bool MeshPaintComponentOwner::Init()
	{
		SetComponentOwner<MeshPaintComponent>({ .Destructor = &MeshPaintComponentOwner::MeshPaintDestructor });
		return true;
	}

	void MeshPaintComponentOwner::Tick(uint64 modFrameIndex)
	{
		s_ModFrameIndex = modFrameIndex;
		auto& resourcesToDestroy = s_ResourcesToRemove[s_ModFrameIndex];
		if (!resourcesToDestroy.IsEmpty())
		{
			for (uint32 i = 0; i < resourcesToDestroy.GetSize(); i++)
			{
				SAFERELEASE(resourcesToDestroy[i]);
			}
			resourcesToDestroy.Clear();
		}
	}

	void MeshPaintComponentOwner::MeshPaintDestructor(MeshPaintComponent& meshPaintComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		s_ResourcesToRemove[s_ModFrameIndex].PushBack(meshPaintComponent.pTexture);
		s_ResourcesToRemove[s_ModFrameIndex].PushBack(meshPaintComponent.pTextureView);
	}
}