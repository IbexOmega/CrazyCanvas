#pragma once
#include "IDeviceChild.h"

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
		}

		virtual ~DeviceChildBase() = default;

		virtual uint64 Release() override
		{
			//TODO: This needs to be synced with a mutex of some kind
			uint64 strongReferences = --m_StrongReferences;
			if (strongReferences < 1)
			{
				delete this;
			}

			return strongReferences;
		}

		virtual uint64 AddRef() override
		{
			//TODO: This needs to be syncted with a mutex of some kind
			return ++m_StrongReferences;
		}

	protected:
		const TGraphicsDevice* const m_pDevice;

	private:
		uint64 m_StrongReferences;
	};
}