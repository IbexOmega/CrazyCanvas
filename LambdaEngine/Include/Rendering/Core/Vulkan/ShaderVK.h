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

		bool Init(const ShaderDesc& desc);

		void FillSpecializationInfo(VkSpecializationInfo& specializationInfo, std::vector<VkSpecializationMapEntry>& specializationEntries) const;
		void FillShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const VkSpecializationInfo& specializationInfo) const;

		//IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		FORCEINLINE VkShaderModule GetModule()
		{
			return m_Module;
		}

		FORCEINLINE virtual ShaderDesc GetDesc() const override final
		{
			return m_Desc;
		}

		FORCEINLINE virtual uint64 GetHandle() const
		{
			return (uint64)m_Module;
		}

	private:
		VkShaderModule	m_Module	= VK_NULL_HANDLE;
		ShaderDesc		m_Desc;
	};
}