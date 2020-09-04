#include "Core/RefCountedObject.h"

#include <mutex>

namespace LambdaEngine
{
	RefCountedObject::RefCountedObject()
		: m_Lock()
	{
		AddRef();
	}

	uint64 RefCountedObject::AddRef()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		return ++m_StrongReferences;
	}

	uint64 RefCountedObject::Release()
	{
		uint64 references = 0;
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			references = --m_StrongReferences;
		}
		
		if (references < 1)
		{
			delete this;
		}
		
		return references;
	}
}
