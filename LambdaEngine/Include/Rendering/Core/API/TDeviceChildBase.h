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
		TDeviceChildBase(const TGraphicsDevice* pDevice)
			: IBase(),
			m_pDevice(pDevice)
		{
            constexpr uint32 sizeInBytes = sizeof(char) * MAX_DEVICE_CHILD_NAME_LENGTH;
            m_pDebugName = (char*)malloc(sizeInBytes);
            
			ZERO_MEMORY(m_pDebugName, sizeInBytes);

			AddRef();
		}

		virtual ~TDeviceChildBase()
		{
            if (m_pDebugName)
            {
                free((void*)m_pDebugName);
                m_pDebugName = nullptr;
            }
            
			m_StrongReferences	= 0;
		}

		virtual uint64 Release() override
		{
            uint64 strongReferences;
            {
                std::scoped_lock<SpinLock> lock(m_Lock);
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
            std::scoped_lock<SpinLock> lock(m_Lock);
			return ++m_StrongReferences;
		}
        
		virtual void SetName(const char* pName) override
		{
			strncpy(m_pDebugName, pName, MAX_DEVICE_CHILD_NAME_LENGTH);
		}

        FORCEINLINE virtual const IGraphicsDevice* GetDevice() const override
        {
            //Cast the device to the correct type, this way we do not actually need to include any implementation.
            //Not the prettiest solution but it works
            return reinterpret_cast<const IGraphicsDevice*>(m_pDevice);
        }

	protected:
		const TGraphicsDevice* const	m_pDevice       = nullptr;
		char*							m_pDebugName    = nullptr;

	private:
        SpinLock    m_Lock;
		uint64      m_StrongReferences = 0;
	};
}
