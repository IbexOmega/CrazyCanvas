#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Resources/ResourceManager.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/EntityMaskManager.h"

using namespace LambdaEngine;

MeshPaintComponent MeshPaint::CreateComponent(Entity entity, const std::string& textureName, uint32 width, uint32 height)
{
	MeshPaintComponent meshPaintComponent;
	char* data = DBG_NEW char[width * height * 4];
	memset(data, 0, width * height * 4);
	meshPaintComponent.UnwrappedTexture = ResourceManager::LoadTextureFromMemory(textureName, data, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, false);
	SAFEDELETE_ARRAY(data);

	DrawArgExtensionData drawArgExtensionData = {};
	drawArgExtensionData.TextureCount = 1;
	drawArgExtensionData.ppTextures[0] = ResourceManager::GetTexture(meshPaintComponent.UnwrappedTexture);
	drawArgExtensionData.ppTextureViews[0] = ResourceManager::GetTextureView(meshPaintComponent.UnwrappedTexture);
	drawArgExtensionData.ppSamplers[0] = Sampler::GetLinearSampler();

	EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), &drawArgExtensionData);

	return meshPaintComponent;
}