#pragma once
#include "Rendering/Core/API/IShader.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ShaderVK : public TDeviceChildBase<GraphicsDeviceVK, IShader>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IShader>;

	public:
		ShaderVK(const GraphicsDeviceVK* pDevice);
		~ShaderVK();

		bool Init(const ShaderDesc* pDesc);

		FORCEINLINE VkShaderModule GetShaderModule() const
		{
			return m_Module;
		}

		FORCEINLINE const char* GetEntryPoint() const
		{
			return m_Desc.pEntryPoint;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		// IShader interface
		FORCEINLINE virtual ShaderDesc GetDesc() const override final
		{
			return m_Desc;
		}

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Module);
		}		

	private:
		VkShaderModule	m_Module	= VK_NULL_HANDLE;
		ShaderDesc		m_Desc;
	};
}
