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
	};

	class SBT : public DeviceChild
	{
	public:
		DECL_INTERFACE(SBT);

	protected:
		String m_DebugName;
	};
}