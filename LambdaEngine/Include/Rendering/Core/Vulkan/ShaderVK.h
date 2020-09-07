#pragma once
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ShaderVK : public TDeviceChildBase<GraphicsDeviceVK, Shader>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Shader>;

	public:
		ShaderVK(const GraphicsDeviceVK* pDevice);
		~ShaderVK();

		bool Init(const ShaderDesc* pDesc);

		FORCEINLINE VkShaderModule GetShaderModule() const
		{
			return m_Module;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Shader interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Module);
		}		

	private:
		VkShaderModule m_Module = VK_NULL_HANDLE;
	};
}
