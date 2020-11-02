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
		* Creates a BLAS and SBT Record for the given Vertex- and Index Buffer if index == BLAS_UNINITIALIZED_INDEX, it then generates a new index which gets stored in the index variable.
		* You are exptected to save this index variable and use it to update/remove this BLAS.

		* If index is not equal to BLAS_UNINITIALIZED_INDEX it must correspond to a valid BLAS.
		* If it corresponds to a valid BLAS it rebuilds that BLAS with pVertexBuffer and pIndexBuffer.
		* You must make sure that vertexCount and indexCount are the same as they were when you first called BuildTriBLAS with this index and allowUpdate must have been true.
		*/
		void BuildTriBLAS(uint32& index, Buffer* pVertexBuffer, Buffer* pIndexBuffer, uint32 vertexCount, uint32 indexCount, bool allowUpdate);

		/*
		* Releases the BLAS corresponding to index.
		* Index must be valid.
		* It is the callers responsibility to make sure all Instances that reference this BLAS are removed prior to calling ReleaseBLAS.
		*/
		void ReleaseBLAS(uint32 index);

		/*
		* Adds a new Instance to the TLAS.
		*	blasIndex - An index corresponding to a valid BLAS.
		*	transform - A valid transform
		*	customIndex - Lower 24 bits will be copied into the instance
		*	hitMask - A hit mask which will be set in the instance and can be used when tracing rays to only hit specific instances
		*	flags - Flags which will be set in the instance
		*/
		uint32 AddInstance(uint32 blasIndex, const glm::mat4& transform, uint32 customIndex, uint8 hitMask, FAccelerationStructureInstanceFlags flags);

		/*
		* Removes an Instance from the TLAS.
		*/
		void RemoveInstance(uint32 instanceIndex);

		/*
		* Updates the transform of the Instance which corresponds to instanceIndex.
		*/
		void UpdateInstanceTransform(uint32 instanceIndex, const glm::mat4& transform);

		/*
		* Loops through each instance and runs updateFunc for it.
		*/
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