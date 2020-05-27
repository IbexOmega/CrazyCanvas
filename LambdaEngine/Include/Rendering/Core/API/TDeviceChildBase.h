#pragma once
#include "IDeviceChild.h"

#include "Threading/API/SpinLock.h"

#include <mutex>
#include <string.h>

#define MAX_DEVICE_CHILD_NAME_LENGTH 256

namespace LambdaEngine
{
	template <typename TGraphicsDevice, typename IBase>
	class TDeviceChildBase : public IBase
	{
	public:
        DECL_UNIQUE_CLASS(TDeviceChildBase);

        TDeviceChildBase(const TGraphicsDevice* pDevice)
            : IBase(),
            m_pDevice(pDevice)
        {
            constexpr uint32 sizeInBytes = sizeof(char) * MAX_DEVICE_CHILD_NAME_LENGTH;
            m_pDebugName = reinterpret_cast<char*>(Malloc::AllocateDbg(sizeInBytes, __FILE__, __LINE__));
            
            ZERO_MEMORY(m_pDebugName, sizeInBytes);

            AddRef();
        }

        virtual ~TDeviceChildBase()
        {
            if (m_pDebugName)
            {
                Malloc::Free(reinterpret_cast<void*>(m_pDebugName));
                m_pDebugName = nullptr;
            }
            
            m_StrongReferences = 0;
        }

        
		virtual uint64 Release() override
		{
            uint64 strongReferences;
            {
                std::scoped_lock<SpinLock> lock(m_RefLock);
                strongReferences = --m_StrongReferences;
            }
            
			if (strongReferences < 1)
			{
                delete this;
			}

			return strongReferences;
		}

		virtual uint64 AddRef() override
		{
            std::scoped_lock<SpinLock> lock(m_RefLock);
			return ++m_StrongReferences;
		}
        
		virtual void SetName(const char* pName) override
		{
            std::scoped_lock<SpinLock> lock(m_DebugNameLock);
			strncpy(m_pDebugName, pName, MAX_DEVICE_CHILD_NAME_LENGTH);
		}

        FORCEINLINE virtual const IGraphicsDevice* GetDevice() const override
        {
            return reinterpret_cast<const IGraphicsDevice*>(m_pDevice);
        }

	protected:
		const TGraphicsDevice* const m_pDevice = nullptr;
		
        char*	 m_pDebugName = nullptr;
        SpinLock m_DebugNameLock;

	private:
        SpinLock m_RefLock;
		uint64   m_StrongReferences = 0;
	};
}
