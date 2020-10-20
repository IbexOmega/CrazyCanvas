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
	MeshPaintComponent MeshPaint::CreateComponent(Entity entity, const std::string& textureName, uint32 width, uint32 height)
	{
		MeshPaintComponent meshPaintComponent = {};
		char* pData = DBG_NEW char[width * height * 4];
		memset(pData, 0, width * height * 4);

		meshPaintComponent.pTexture = ResourceLoader::LoadTextureArrayFromMemory(textureName, reinterpret_cast<void**>(&pData), 1, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, true);

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= textureName + " Texture View";
		textureViewDesc.pTexture		= meshPaintComponent.pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= textureViewDesc.pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= textureViewDesc.pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		meshPaintComponent.pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);;

		DrawArgExtensionData drawArgExtensionData = {};
		drawArgExtensionData.TextureCount = 1;
		drawArgExtensionData.ppTextures[0]		= meshPaintComponent.pTexture;
		drawArgExtensionData.ppTextureViews[0]	= meshPaintComponent.pTextureView;
		drawArgExtensionData.ppSamplers[0]		= Sampler::GetNearestSampler();

		TextureViewDesc mipZeroTextureViewDesc = {};
		mipZeroTextureViewDesc.DebugName		= textureName + " Mip Zero Texture View";
		mipZeroTextureViewDesc.pTexture			= meshPaintComponent.pTexture;
		mipZeroTextureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		mipZeroTextureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		mipZeroTextureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		mipZeroTextureViewDesc.MiplevelCount	= 1;
		mipZeroTextureViewDesc.ArrayCount		= textureViewDesc.pTexture->GetDesc().ArrayCount;
		mipZeroTextureViewDesc.Miplevel			= 0;
		mipZeroTextureViewDesc.ArrayIndex		= 0;

		drawArgExtensionData.ppMipZeroTextureViews[0] = RenderAPI::GetDevice()->CreateTextureView(&mipZeroTextureViewDesc);

		EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), &drawArgExtensionData);

		SAFEDELETE_ARRAY(pData);
		return meshPaintComponent;
	}
}
