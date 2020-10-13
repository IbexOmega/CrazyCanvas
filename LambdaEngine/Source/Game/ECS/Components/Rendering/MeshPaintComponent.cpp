#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Resources/ResourceLoader.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/EntityMaskManager.h"
#include "Rendering/RenderAPI.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"

using namespace LambdaEngine;

MeshPaintComponent MeshPaint::CreateComponent(Entity entity, const std::string& textureName, uint32 width, uint32 height)
{
	MeshPaintComponent meshPaintComponent = {};
	char* pData = DBG_NEW char[width * height * 4];
	memset(pData, 0, width * height * 4);
	
	TextureViewDesc textureViewDesc = {};
	textureViewDesc.DebugName		= textureName + " Texture View";
	textureViewDesc.pTexture		= ResourceLoader::LoadTextureArrayFromMemory(textureName, reinterpret_cast<void**>(&pData), 1, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, false);
	textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
	textureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
	textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
	textureViewDesc.MiplevelCount	= textureViewDesc.pTexture->GetDesc().Miplevels;
	textureViewDesc.ArrayCount		= textureViewDesc.pTexture->GetDesc().ArrayCount;
	textureViewDesc.Miplevel		= 0;
	textureViewDesc.ArrayIndex		= 0;

	DrawArgExtensionData drawArgExtensionData = {};
	drawArgExtensionData.TextureCount = 1;
	drawArgExtensionData.ppTextures[0]		= textureViewDesc.pTexture;
	drawArgExtensionData.ppTextureViews[0]	= RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
	drawArgExtensionData.ppSamplers[0]		= Sampler::GetLinearSampler();

	EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), &drawArgExtensionData);

	SAFEDELETE_ARRAY(pData);
	return meshPaintComponent;
}