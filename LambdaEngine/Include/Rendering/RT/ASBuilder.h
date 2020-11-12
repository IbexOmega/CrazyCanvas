#pragma once

#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/CommandList.h"

namespace LambdaEngine
{
	constexpr const uint32 BLAS_UNINITIALIZED_INDEX = UINT32_MAX;

	struct ASInstanceDesc
	{
		uint32								BlasIndex	= BLAS_UNINITIALIZED_INDEX;
		const glm::mat4&					Transform;
		uint32								CustomIndex	= 0;
		uint8								HitMask		= 0;
		FAccelerationStructureInstanceFlags	Flags		= 0;
	};

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
		virtual bool RenderGraphPostInit() override final;

		/*
		* Creates a BLAS and SBT Record for the given Vertex- and Index Buffer if index == BLAS_UNINITIALIZED_INDEX, it then generates a new index which gets stored in the index variable.
		* You are exptected to save this index variable and use it to update/remove this BLAS.

		* If index is not equal to BLAS_UNINITIALIZED_INDEX it must correspond to a valid BLAS.
		* If it corresponds to a valid BLAS it rebuilds that BLAS with pVertexBuffer and pIndexBuffer.
		* You must make sure that vertexCount and indexCount are the same as they were when you first called BuildTriBLAS with this index and allowUpdate must have been true.
		*/
		void BuildTriBLAS(uint32& blasIndex, uint32 hitGroupIndex, Buffer* pVertexBuffer, Buffer* pIndexBuffer, uint32 vertexCount, uint32 vertexSize, uint32 indexCount, bool allowUpdate);

		/*
		* Releases the BLAS corresponding to index.
		* Index must be valid.
		* It is the callers responsibility to make sure all Instances that reference this BLAS are removed prior to calling ReleaseBLAS.
		*/
		void ReleaseBLAS(uint32 index);

		/*
		* Adds a new Instance to the TLAS. 
		* Below the variables in ASInstanceDesc are explained.
		*	blasIndex - An index corresponding to a valid BLAS.
		*	transform - A valid transform
		*	customIndex - Lower 24 bits will be copied into the instance
		*	hitMask - A hit mask which will be set in the instance and can be used when tracing rays to only hit specific instances
		*	flags - Flags which will be set in the instance
		*	return - The ASInstance ID that corresponds to this new instance
		*/
		uint32 AddInstance(const ASInstanceDesc& asInstanceDesc);

		/*
		* Adds multiple instances, the returned ASInstance IDs are stored in asInstanceIDs and are guaranteed to be in the same order as asInstanceData.
		*/
		void AddInstances(const TArray<ASInstanceDesc>& asInstanceDescriptions, TArray<uint32>& asInstanceIDs);

		/*
		* Removes an Instance from the TLAS.
		*/
		void RemoveInstance(uint32 instanceIndex);

		/*
		* Updates the transform of the Instance which corresponds to instanceIndex.
		*/
		void UpdateInstanceTransform(uint32 instanceIndex, const glm::mat4& transform);

		/*
		* Finds the instance represented by instanceIndex and runs updateFunc for it.
		*/
		void UpdateInstance(uint32 instanceIndex, std::function<void(AccelerationStructureInstance&)> updateFunc);

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

		const AccelerationStructureInstance& GetInstance(uint32 instanceIndex) const;

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
		bool CreateDummyBuffers();

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
		TArray<uint32> m_HitGroupIndices;	//Hit Group Index
		bool m_SBTRecordsDirty = false;

		//TLAS
		TArray<uint32> m_FreeInstanceIndices;				//Keeps track of holes in m_InstanceIndices
		bool m_InstanceIndicesChanged = false;
		TArray<uint32> m_InstanceIndices;					//Maps Instance Index returned from AddInstance() to index in m_Instances
		Buffer** m_ppInstanceIndicesStagingBuffers = nullptr;
		Buffer* m_pInstanceIndicesBuffer = nullptr;
		TArray<AccelerationStructureInstance> m_Instances;	//Dense Array of Instances
		Buffer** m_ppInstanceStagingBuffers = nullptr;
		Buffer* m_pInstanceBuffer = nullptr;
		uint32 m_MaxSupportedTLASInstances = 0;
		uint32 m_BuiltTLASInstanceCount = 0;
		AccelerationStructure* m_pTLAS = nullptr;

		//Utility
		TArray<DeviceChild*>* m_pResourcesToRemove = nullptr;

		//Threading
		mutable SpinLock m_Lock;
	};
}