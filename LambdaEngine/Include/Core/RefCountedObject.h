#pragma once
#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class RefCountedObject
	{
	public:
		DECL_UNIQUE_CLASS(RefCountedObject);
		
		RefCountedObject();
		virtual ~RefCountedObject() = default;
		
		/*
		* Decrements the referencecounter for the object
		*	return - Returns the new referencecount for the object
		*/
		virtual uint64 AddRef();

		/*
		* Increases the referencecounter for the object
		*	return - Returns the new referencecount for the object
		*/
		virtual uint64 Release();
		
		FORCEINLINE uint64 GetRefCount() const
		{
			return m_StrongReferences;
		}
		
	private:
		uint64		m_StrongReferences = 0;
		SpinLock	m_Lock;
	};
}
