#pragma once
#include "DeviceChild.h"

namespace LambdaEngine
{
	template <typename TGraphicsDevice, typename TBase>
	class TDeviceChildBase : public TBase
	{
	public:
		DECL_UNIQUE_CLASS(TDeviceChildBase);

		TDeviceChildBase(const TGraphicsDevice* pDevice)
			: TBase(),
			m_pDevice(pDevice)
		{
			static_assert(std::is_base_of<DeviceChild, TBase>());
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
