#pragma once
#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	/*
	* RefCountedObject
	*/
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
		virtual uint64 AddRef() const;

		/*
		* Increases the referencecounter for the object
		*	return - Returns the new referencecount for the object
		*/
		virtual uint64 Release() const;
		
		FORCEINLINE uint64 GetRefCount() const
		{
			return m_StrongReferences;
		}
		
	private:
		mutable uint64 m_StrongReferences = 0;
		mutable SpinLock m_Lock;
	};
}
