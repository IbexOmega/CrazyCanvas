#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	struct SBTDesc
	{
		String				DebugName		= "";
		PipelineState*		pPipelineState	= nullptr;
		TArray<SBTRecord>	SBTRecords;
		TArray<uint32>		HitGroupIndices;
	};

	class SBT : public DeviceChild
	{
	public:
		DECL_INTERFACE(SBT);

		virtual bool Build(CommandList* pCommandList, TArray<DeviceChild*>& removedDeviceResources, const SBTDesc* pDesc) = 0;

	protected:
		String m_DebugName;
	};
}