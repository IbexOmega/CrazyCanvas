#pragma once
#include "IDeviceChild.h"

#include "Threading/SpinLock.h"

#include <mutex>

namespace LambdaEngine
{
	template <typename TGraphicsDevice, typename IBase>
	class DeviceChildBase : public IBase
	{
	public:
		DeviceChildBase(const TGraphicsDevice* pDevice)
			: IBase(),
			m_pDevice(pDevice),
			m_StrongReferences(0)
		{
			AddRef();
		}

		virtual ~DeviceChildBase() = default;

		virtual uint64 Release() override
		{
            std::scoped_lock<SpinLock> lock(m_Lock);
            
			uint64 strongReferences = --m_StrongReferences;
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
        
        FORCEINLINE virtual const IGraphicsDevice* GetDevice() const override
        {
            //Cast the device to the correct type, this way we do not actually need to include any implementation.
            //Not the prettiest solution but it works
            return reinterpret_cast<const IGraphicsDevice*>(m_pDevice);
        }

	protected:
		const TGraphicsDevice* const m_pDevice;

	private:
        SpinLock    m_Lock;
		uint64      m_StrongReferences;
	};
}
