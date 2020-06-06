#pragma once
#include "DeviceChild.h"

namespace LambdaEngine
{
	template <typename TGraphicsDevice, typename Base>
	class TDeviceChildBase : public Base
	{
	public:
		DECL_UNIQUE_CLASS(TDeviceChildBase);

		TDeviceChildBase(const TGraphicsDevice* pDevice)
			: Base(),
			m_pDevice(pDevice)
		{
			// Base needs to inherit DeviceChild
			Base::AddRef();
		}

		~TDeviceChildBase() = default;

		// DeviceChild interface
		virtual const GraphicsDevice* GetDevice() const override
		{
			return reinterpret_cast<const GraphicsDevice*>(m_pDevice);
		}

	protected:
		const TGraphicsDevice* const m_pDevice = nullptr;
	};
}
