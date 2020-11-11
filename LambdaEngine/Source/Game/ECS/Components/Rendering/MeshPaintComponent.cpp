#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Resources/ResourceLoader.h"
#include "Rendering/RenderGraphTypes.h"
#include "Rendering/EntityMaskManager.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"

namespace LambdaEngine
{
	MeshPaintComponent MeshPaint::CreateComponent(Entity entity, const std::string& textureName, uint32 width, uint32 height, bool generateMips)
	{
		MeshPaintComponent meshPaintComponent = {};
		char* pData = DBG_NEW char[width * height * 2];
		memset(pData, 0, width * height * 2);

		meshPaintComponent.pTexture = ResourceLoader::LoadTextureArrayFromMemory(
			textureName, 
			reinterpret_cast<void**>(&pData), 
			1, 
			width, 
			height, 
			EFormat::FORMAT_R8G8_UINT, 
			FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, 
			generateMips,
			false);

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= textureName + " Texture View";
		textureViewDesc.pTexture		= meshPaintComponent.pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= EFormat::FORMAT_R8G8_UINT;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;
		meshPaintComponent.pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		DrawArgExtensionData drawArgExtensionData = {};
		drawArgExtensionData.TextureCount = 1;
		drawArgExtensionData.ppTextures[0]				= meshPaintComponent.pTexture;
		drawArgExtensionData.ppTextureViews[0]			= meshPaintComponent.pTextureView;
		drawArgExtensionData.ppSamplers[0]				= Sampler::GetNearestSampler();

		EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), &drawArgExtensionData);

		SAFEDELETE_ARRAY(pData);
		return meshPaintComponent;
	}
}
