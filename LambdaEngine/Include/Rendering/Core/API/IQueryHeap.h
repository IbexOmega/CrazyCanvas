#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct QueryHeapDesc
	{
		const char* pName					= "";
		EQueryType	Type					= EQueryType::QUERY_TYPE_NONE;
		uint32		QueryCount				= 0;
		uint32		PipelineStatisticsFlags = 0;
	};

	class IQueryHeap : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IQueryHeap);

		/*
		* Get the results from the query heap
		*
		* firstQuery	- Index to the first query in the heap starting at zero
		* queryCount	- Number of queries to get results from
		* pData			- Array of type uint64 that the results will be written to, must have size of queryCount
		*/
		virtual bool GetResults(uint32 firstQuery, uint32 queryCount, uint64* pData) const = 0;

		/*
		* Returns the API-specific handle to the underlaying QueryHeap-resource
		*
		* return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64			GetHandle() const = 0;
		virtual QueryHeapDesc	GetDesc()	const = 0;
	};
}