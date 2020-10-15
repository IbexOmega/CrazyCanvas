#pragma once
#include "Types.h"
#include "Defines.h"

/*
* Delete and Release
*/

#define DELETE_OBJECT(object)	delete(object); (object) = nullptr
#define SAFEDELETE(object)		if ((object))	{ DELETE_OBJECT(object); }

#define DELETE_ARRAY(array)		delete[](array); (array) = nullptr
#define SAFEDELETE_ARRAY(array)	if ((array))	{ DELETE_ARRAY(array); }

#define RELEASE(object)			(object)->Release(); (object) = nullptr
#define SAFERELEASE(object)		if ((object))	{ RELEASE(object); }

namespace LambdaEngine
{
	/*
	* FMemoryDebugFlags
	*/

	enum FMemoryDebugFlags : uint16
	{
		MEMORY_DEBUG_FLAGS_NONE				= 0,
		MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT	= FLAG(1),
		MEMORY_DEBUG_FLAGS_LEAK_CHECK		= FLAG(2),
	};

	/*
	* Malloc
	*/

	class LAMBDA_API Malloc
	{
	public:
		DECL_STATIC_CLASS(Malloc);
		
		static void* Allocate(uint64 sizeInBytes);
		static void* Allocate(uint64 sizeInBytes, uint64 alignment);

		template<typename T>
		static T* AllocateType(uint64 sizeInBytes)
		{
			return reinterpret_cast<T*>(Allocate(sizeInBytes));
		}

		template<typename T>
		static T* AllocateType()
		{
			return reinterpret_cast<T*>(Allocate(sizeof(T)));
		}

		static void* AllocateDbg(uint64 sizeInBytes, const char* pFileName, int32 lineNumber);
		static void* AllocateDbg(uint64 sizeInBytes, uint64 alignment, const char* pFileName, int32 lineNumber);
		
		static void Free(void* pPtr);
		
		static void SetDebugFlags(uint16 debugFlags);

	private:
		static void* AllocateProtected(uint64 sizeInBytes);
		static void* AlignAddress(void* pAddress, uint64 alignment);

		static void SetAllocationFlags(void* pAllocation, uint16 padding);
		static uint16 GetAllocationFlags(void* pAllocation);
		static uint16 GetAllocationPadding(void* pAllocation);

	private:
		static uint16 s_DebugFlags;
	};
}