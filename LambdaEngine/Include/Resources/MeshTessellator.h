#pragma once
#include "Resources/Mesh.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Shader.h"

namespace LambdaEngine
{
	class MeshTessellator
	{
	private:
		struct SPushConstantData
		{
			uint32 triangleCount		= 0;
			uint32 maxTesselationLevels = 0;
			uint32 outerBorder			= 0;
		};

	public:
		void Init();
		void Release();

		void Tessellate(Mesh* pMesh);

		static MeshTessellator& GetInstance() { return s_Instance; }

	private:
		void CreateDummyRenderTarget();

		void CreateAndCopyInBuffer(CommandList* pCommandList, Buffer** inBuffer, Buffer** inStagingBuffer, void* data, uint64 size, const String& name, FBufferFlags flags);
		void CreateAndClearOutBuffer(CommandList* pCommandList, Buffer** outBuffer, Buffer** outFirstStagingBuffer, Buffer** outSecondStagingBuffer, uint64 size, uint32 clearData, const String& name);

	private:
		CommandAllocator* m_pCommandAllocator;
		CommandList* m_pCommandList;

		DescriptorHeap* m_pDescriptorHeap;
		DescriptorSet* m_pInDescriptorSet;
		DescriptorSet* m_pOutDescriptorSet;

		Fence* m_pFence;

		PipelineLayout* m_pPipelineLayout;
		GUID_Lambda m_ShaderGUID;

		RenderPass* m_RenderPass;
		GUID_Lambda m_pPipelineStateID;

		Texture* m_pDummyTexture;
		TextureView* m_pDummyTextureView;

		Buffer* m_pInVertexStagingBuffer = nullptr;
		Buffer* m_pInVertexBuffer = nullptr;

		Buffer* m_pInIndicesStagingBuffer = nullptr;
		Buffer* m_pInIndicesBuffer = nullptr;

		Buffer* m_pOutVertexFirstStagingBuffer = nullptr;
		Buffer* m_pOutVertexSecondStagingBuffer = nullptr;
		Buffer* m_pOutVertexBuffer = nullptr;

		Buffer* m_pOutIndicesFirstStagingBuffer = nullptr;
		Buffer* m_pOutIndicesSecondStagingBuffer = nullptr;
		Buffer* m_pOutIndicesBuffer = nullptr;

	private:
		static MeshTessellator s_Instance;

	};
}