#include "Game/ECS/ComponentOwners/Rendering/MeshPaintComponentOwner.h"

#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"

namespace LambdaEngine
{
	MeshPaintComponentOwner MeshPaintComponentOwner::s_Instance;

	bool MeshPaintComponentOwner::Init()
	{
		SetComponentOwner<MeshPaintComponent>({ std::bind_front(&MeshPaintComponentOwner::MeshPaintDestructor, this) });
		return true;
	}

	void MeshPaintComponentOwner::MeshPaintDestructor(MeshPaintComponent& meshPaintComponent)
	{
		SAFERELEASE(meshPaintComponent.pTexture);
		SAFERELEASE(meshPaintComponent.pTextureView);
		SAFERELEASE(meshPaintComponent.pMipZeroTextureView);
	}
}