#include "Game/ECS/ComponentOwners/Rendering/MeshPaintComponentOwner.h"

#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Buffer.h"

namespace LambdaEngine
{
	MeshPaintComponentOwner MeshPaintComponentOwner::s_Instance;

	bool MeshPaintComponentOwner::Init()
	{
		SetComponentOwner<MeshPaintComponent>({ .Destructor = &MeshPaintComponentOwner::MeshPaintDestructor });
		return true;
	}

	bool MeshPaintComponentOwner::Release()
	{
		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			auto& resourcesToRelease = m_ResourcesToRelease[b];
			for (uint32 i = 0; i < resourcesToRelease.GetSize(); i++)
			{
				SAFERELEASE(resourcesToRelease[i]);
			}
			resourcesToRelease.Clear();
		}

		return true;
	}

	void MeshPaintComponentOwner::Tick(uint64 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;
		auto& resourcesToRelease = m_ResourcesToRelease[m_ModFrameIndex];
		if (!resourcesToRelease.IsEmpty())
		{
			for (uint32 i = 0; i < resourcesToRelease.GetSize(); i++)
			{
				SAFERELEASE(resourcesToRelease[i]);
			}
			resourcesToRelease.Clear();
		}
	}

	void MeshPaintComponentOwner::ReleaseMeshPaintComponent(MeshPaintComponent& meshPaintComponent)
	{
		TArray<DeviceChild*>& resourcesToRelease = m_ResourcesToRelease[m_ModFrameIndex];
		resourcesToRelease.PushBack(meshPaintComponent.pTexture);
		resourcesToRelease.PushBack(meshPaintComponent.pTextureView);
		resourcesToRelease.PushBack(meshPaintComponent.pReadBackBuffer);
	}

	void MeshPaintComponentOwner::MeshPaintDestructor(MeshPaintComponent& meshPaintComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		MeshPaintComponentOwner* pMeshPaintComponentOwner = GetInstance();
		pMeshPaintComponentOwner->ReleaseMeshPaintComponent(meshPaintComponent);
	}
}