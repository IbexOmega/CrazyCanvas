#pragma once
#include "Resources/Mesh.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Shader.h"

#include "Rendering/Core/API/CommandList.h"

namespace LambdaEngine
{
	class MeshTessellator
	{
	private:
		struct SPushConstantData
		{
			uint32 OutputMaxVertexCount	= 0;
		};

		struct CalculationData
		{
			glm::mat4 ScaleMatrix;
			uint32 PrimitiveCounter;
			float MaxInnerLevelTess;
			float MaxOuterLevelTess;
			float Padding;
		};

	public:
		void Init();
		void Release();

		void Tessellate(Mesh* pMesh);
		uint32 MergeDuplicateVertices(const TArray<Vertex>& unmergedVertices, Mesh* pMesh);
		void ReleaseTessellationBuffers();

		static MeshTessellator& GetInstance() { return s_Instance; }

	private:
		void SubMeshTessellation(uint32 indexCount, uint32 indexOffset, uint64 outputSize, uint64& signalValue);

		void CreateDummyRenderTarget();

		void CreateAndCopyInBuffer(CommandList* pCommandList, Buffer** ppInBuffer, Buffer** ppInStagingBuffer, void* data, uint64 size, const String& name, FBufferFlags flags);
		void CreateOutBuffer(CommandList* pCommandList, Buffer** ppOutBuffer, Buffer** ppOutSecondStagingBuffer, uint64 size, const String& name);

	private:
		/*
		*  Max Inner Tessellation Level for Primitives
		*/
		float m_MaxInnerTessLevel = 0;

		/*
		*  Max Outer Tessellation Levels for Primitives
		*/
		float m_MaxOuterTessLevel = 0;

		/*
		*  Worst case tessellation of primitive triangle count
		*/
		uint32 m_MaxTessellationTriangleCount = 0;

		/*
		*  Max triangle count per tessellation execution
		*/
		uint32 m_MaxTrianglesPerSubTess = 0;

		uint32 m_TotalVerticesAdded = 0;

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

		Buffer* m_pCalculationDataStagingBuffer = nullptr;
		Buffer* m_pCalculationDataBuffer = nullptr;

		Buffer* m_pInVertexStagingBuffer = nullptr;
		Buffer* m_pInVertexBuffer = nullptr;

		Buffer* m_pInIndicesStagingBuffer = nullptr;
		Buffer* m_pInIndicesBuffer = nullptr;

		Buffer* m_pOutVertexSecondStagingBuffer = nullptr;
		Buffer* m_pOutVertexBuffer = nullptr;

		BeginRenderPassDesc m_BeginRenderPassDesc;
		Viewport m_Viewport;
		ScissorRect m_ScissorRect;

	private:
		static MeshTessellator s_Instance;

	};
}