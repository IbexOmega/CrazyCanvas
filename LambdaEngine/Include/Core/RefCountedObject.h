#pragma once
#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
    class RefCountedObject
    {
    public:
        DECL_UNIQUE_CLASS(RefCountedObject);
        
        RefCountedObject();
        virtual ~RefCountedObject()  = default;
        
        uint64 AddRef();
        uint64 Release();
        
        FORCEINLINE uint64 GetRefCount() const
        {
            return m_StrongReferences;
        }
        
    private:
        uint64      m_StrongReferences = 0;
        SpinLock    m_Lock;
    };
}
