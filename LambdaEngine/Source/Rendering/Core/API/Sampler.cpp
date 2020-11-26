#include "PreCompiled.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"

namespace LambdaEngine
{
	Sampler* Sampler::s_pLinearSampler	= nullptr;
	Sampler* Sampler::s_pNearestSampler = nullptr;

	bool Sampler::InitDefaults()
	{
		SamplerDesc samplerLinearDesc = {};
		samplerLinearDesc.DebugName			= "Linear Sampler";
		samplerLinearDesc.MinFilter			= EFilterType::FILTER_TYPE_LINEAR;
		samplerLinearDesc.MagFilter			= EFilterType::FILTER_TYPE_LINEAR;
		samplerLinearDesc.MipmapMode		= EMipmapMode::MIPMAP_MODE_LINEAR;
		samplerLinearDesc.AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerLinearDesc.AddressModeV		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerLinearDesc.AddressModeW		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerLinearDesc.MipLODBias		= 0.0f;
		samplerLinearDesc.AnisotropyEnabled	= true;
		samplerLinearDesc.MaxAnisotropy		= 16;
		samplerLinearDesc.MinLOD			= 0.0f;
		samplerLinearDesc.MaxLOD			= FLT_MAX;

		s_pLinearSampler = RenderAPI::GetDevice()->CreateSampler(&samplerLinearDesc);
		if (s_pLinearSampler == nullptr)
		{
			return false;
		}

		SamplerDesc samplerNearestDesc = {};
		samplerNearestDesc.DebugName			= "Nearest Sampler";
		samplerNearestDesc.MinFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerNearestDesc.MagFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerNearestDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_NEAREST;
		samplerNearestDesc.AddressModeU			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerNearestDesc.AddressModeV			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerNearestDesc.AddressModeW			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
		samplerNearestDesc.MipLODBias			= 0.0f;
		samplerNearestDesc.AnisotropyEnabled	= true;
		samplerNearestDesc.MaxAnisotropy		= 16;
		samplerNearestDesc.MinLOD				= 0.0f;
		samplerNearestDesc.MaxLOD				= FLT_MAX;

		s_pNearestSampler = RenderAPI::GetDevice()->CreateSampler(&samplerNearestDesc);
		if (s_pNearestSampler == nullptr)
		{
			return false;
		}

		return true;
	}

	void Sampler::ReleaseDefaults()
	{
		SAFERELEASE(s_pLinearSampler);
		SAFERELEASE(s_pNearestSampler);
	}
}