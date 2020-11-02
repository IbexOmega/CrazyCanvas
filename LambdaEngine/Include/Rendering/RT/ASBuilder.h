#pragma once

#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/CommandList.h"

namespace LambdaEngine
{
	constexpr const uint32 BLAS_UNINITIALIZED_INDEX = UINT32_MAX;

	class ASBuilder : public CustomRenderer
	{
		struct BLASData
		{
			AccelerationStructure* pBLAS = nullptr;
			uint32 SBTRecordOffset = UINT32_MAX;
		};

	public:
		ASBuilder();
		~ASBuilder();

		virtual bool Init() override final;
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		/*
			Creates a BLAS and SBT Record for the given Vertex- and Index Buffer if index == BLAS_UNINITIALIZED_INDEX, it then generates a unique hash for this BLAS
		*/
		void BuildTriBLAS(uint32& index, Buffer* pVertexBuffer, Buffer* pIndexBuffer, uint32 vertexCount, uint32 indexCount, bool allowUpdate);
		void ReleaseBLAS(uint32 index);

		uint32 AddInstance(uint32 blasIndex, const glm::mat4& transform, uint32 customIndex, uint8 hitMask, FAccelerationStructureFlags flags);
		void RemoveInstance(uint32 instanceIndex);
		void UpdateInstanceTransform(uint32 instanceIndex, const glm::mat4& transform);
		void UpdateInstances(std::function<void(AccelerationStructureInstance&)> updateFunc);

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;

		virtual void Render(
			uint32 modFrameIndex, 
			uint32 backBufferIndex, 
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		AccelerationStructureInstance& GetInstance(uint32 instanceIndex);

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final	{ return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final	{ return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual const String& GetName() const override final 
		{
			static String name = "AS_BUILDER";
			return name;
		}

	private:
		void ReleaseBackBufferBound();

		bool CreateCommandLists();
		bool CreateBuffers();

	private:
		RenderGraph* m_pRenderGraph = nullptr;
		uint32 m_ModFrameIndex = 0;
		uint32 m_BackBufferCount = 0;

		CommandAllocator** m_ppComputeCommandAllocators = nullptr;
		CommandList** m_ppComputeCommandLists = nullptr;

		//BLAS
		TArray<uint32> m_FreeBLASIndices;	//Keeps track of holes in m_BLASes
		TArray<BLASData> m_BLASes;			//Non-Dense Array of BLASData
		TArray<BuildBottomLevelAccelerationStructureDesc> m_DirtyBLASes; //Contains all BLAS build descriptions queued for execution
		
		//SBT
		TArray<uint32> m_FreeSBTIndices;	//Keeps track of holes in m_SBTRecords
		TArray<SBTRecord> m_SBTRecords;		//Non-Dense Array of SBTRecords
		bool m_SBTRecordsDirty = false;

		//TLAS
		TArray<uint32> m_FreeInstanceIndices;				//Keeps track of holes in m_InstanceIndices
		TArray<uint32> m_InstanceIndices;					//Maps Instance Index returned from AddInstance() to index in m_Instances
		TArray<AccelerationStructureInstance> m_Instances;	//Dense Array of Instances
		Buffer** m_ppInstanceBuffers = nullptr;
		uint32 m_MaxSupportedTLASInstances = 0;
		uint32 m_BuiltTLASInstanceCount = 0;
		AccelerationStructure* m_pTLAS = nullptr;

		//Utility
		TArray<DeviceChild*>* m_pResourcesToRemove = nullptr;

		//Threading
		SpinLock m_Lock;
	};
}