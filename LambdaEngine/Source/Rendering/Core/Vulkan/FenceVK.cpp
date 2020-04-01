#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
	FenceVK::FenceVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{

	}
	
	FenceVK::~FenceVK()
	{
		if (m_Semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_pDevice->Device, m_Semaphore, nullptr);
			m_Semaphore = VK_NULL_HANDLE;
		}

		m_FenceValue = 0;
	}
	
	bool FenceVK::Init()
	{


		return false;
	}
	void FenceVK::Wait() const
	{
	}
	void FenceVK::Signal()
	{
	}
}