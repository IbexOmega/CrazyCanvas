#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	enum class EQueryType
	{
		NONE					= 0,
		QUERY_TYPE_TIMESTAMP	= 1,
	};

	struct QueryDesc
	{
		const char* pName	= "";
		EQueryType	Type	= EQueryType::NONE;
	};

	class IQueryHeap : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IQueryHeap);

		virtual QueryDesc GetDesc() const = 0;
	};
}