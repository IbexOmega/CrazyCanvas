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
	MeshPaintComponent MeshPaint::CreateComponent(
		Entity entity, 
		const std::string& textureName, 
		uint32 width, 
		uint32 height,
		bool createMipReadBack)
	{
		MeshPaintComponent meshPaintComponent = {};
		char* pData = DBG_NEW char[width * height * 2];
		memset(pData, 0, width * height * 2);

		meshPaintComponent.pTexture = ResourceLoader::LoadTextureArrayFromMemory(
			textureName,
			reinterpret_cast<void**>(&pData),
			1, width, height,
			EFormat::FORMAT_R8G8_UINT,
			FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, 
			true,
			false);

		VALIDATE(meshPaintComponent.pTexture != nullptr);

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= textureName + " Texture View";
		textureViewDesc.pTexture		= meshPaintComponent.pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= EFormat::FORMAT_R8G8_UINT;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= textureViewDesc.pTexture->GetDesc().Miplevels;
		textureViewDesc.ArrayCount		= textureViewDesc.pTexture->GetDesc().ArrayCount;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;
		meshPaintComponent.pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		TextureViewDesc mipZeroTextureViewDesc = {};
		mipZeroTextureViewDesc.DebugName		= textureName + " Mip Zero Texture View";
		mipZeroTextureViewDesc.pTexture			= meshPaintComponent.pTexture;
		mipZeroTextureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		mipZeroTextureViewDesc.Format			= EFormat::FORMAT_R8G8_UINT;
		mipZeroTextureViewDesc.Type				= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		mipZeroTextureViewDesc.MiplevelCount	= 1;
		mipZeroTextureViewDesc.ArrayCount		= textureViewDesc.pTexture->GetDesc().ArrayCount;
		mipZeroTextureViewDesc.Miplevel			= 0;
		mipZeroTextureViewDesc.ArrayIndex		= 0;
		meshPaintComponent.pMipZeroTextureView	= RenderAPI::GetDevice()->CreateTextureView(&mipZeroTextureViewDesc);

		if (createMipReadBack)
		{
			// Assumes quad textures (32*32)
			constexpr uint32 HEALTH_READ_WIDTH	= 32;
			constexpr uint32 HEALTH_READ_HEIGHT	= 32;
			constexpr uint32 SIZE_IN_BYTES		= HEALTH_READ_WIDTH * HEALTH_READ_HEIGHT * 2;

			BufferDesc healthBufferDesc = {};
			healthBufferDesc.DebugName		= "HealthBuffer";
			healthBufferDesc.SizeInBytes	= SIZE_IN_BYTES;
			healthBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			healthBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST;

			meshPaintComponent.pReadBackBuffer = RenderAPI::GetDevice()->CreateBuffer(&healthBufferDesc);
			VALIDATE(meshPaintComponent.pReadBackBuffer != nullptr);

			void* pCPUMemory = meshPaintComponent.pReadBackBuffer->Map();
			ZERO_MEMORY(pCPUMemory, SIZE_IN_BYTES);
			meshPaintComponent.pReadBackBuffer->Unmap();
		}

		DrawArgExtensionData drawArgExtensionData = {};
		drawArgExtensionData.TextureCount				= 1;
		drawArgExtensionData.ppTextures[0]				= meshPaintComponent.pTexture;
		drawArgExtensionData.ppTextureViews[0]			= meshPaintComponent.pTextureView;
		drawArgExtensionData.ppMipZeroTextureViews[0]	= meshPaintComponent.pMipZeroTextureView;
		drawArgExtensionData.ppReadBackBuffers[0]		= meshPaintComponent.pReadBackBuffer;
		drawArgExtensionData.ppSamplers[0]				= Sampler::GetNearestSampler();

		EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), &drawArgExtensionData);

		SAFEDELETE_ARRAY(pData);
		return meshPaintComponent;
	}
}
