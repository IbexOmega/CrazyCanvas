#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct QueryHeapDesc
	{
		String		DebugName				= "";
		EQueryType	Type					= EQueryType::QUERY_TYPE_NONE;
		uint32		QueryCount				= 0;
		uint32		PipelineStatisticsFlags = 0;
	};

	struct QueryHeapAvailabilityResult
	{
		uint64 Result		= 0;
		uint64 Availability	= 0;
	};

	class QueryHeap : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(QueryHeap);

		/*
		* Get the results from the query heap
		*	 firstQuery	- Index to the first query in the heap starting at zero
		*	 queryCount	- Number of queries to get results from
		*	 pData		- Array of type uint64 that the results will be written to, must have size of queryCount
		*/
		virtual bool GetResults(uint32 firstQuery, uint32 queryCount, uint64 dataSize, uint64* pData) const = 0;

		/*
		* Get the results from the query heap. The dataSize should be twice as large as the amount of queries.
		* This is because the availability is also collected with this function, and it is laid out in memory as follows:
		* [[result0,availability0], [result1,availability1]] where each variable is an uint64
		*	 firstQuery	- Index to the first query in the heap starting at zero
		*	 queryCount	- Number of queries to get results from
		*	 pData		- Array of type QueryHeapAvailabilityResult that the results will be written to, must have size of queryCount
		*/
		virtual bool GetResultsAvailable(uint32 firstQuery, uint32 queryCount, uint64 dataSize, QueryHeapAvailabilityResult* pData) const = 0;

		/*
		* Returns the API-specific handle to the underlaying QueryHeap-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		FORCEINLINE const QueryHeapDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		QueryHeapDesc m_Desc;
	};
}