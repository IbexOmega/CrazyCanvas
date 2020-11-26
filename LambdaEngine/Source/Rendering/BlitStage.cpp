#include "Rendering/BlitStage.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Texture.h"

namespace LambdaEngine
{
	BlitStage::~BlitStage()
	{
		if (m_ppGraphicCommandAllocators != nullptr && m_ppGraphicCommandLists != nullptr)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppGraphicCommandLists[b]);
				SAFERELEASE(m_ppGraphicCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppGraphicCommandLists);
			SAFEDELETE_ARRAY(m_ppGraphicCommandAllocators);
		}
	}

	bool BlitStage::Init()
	{
		m_BlitDescriptions =
		{
			BlitDescription
			{
				.SrcName = "G_BUFFER_LINEAR_Z",
				.DstName = "G_BUFFER_PREV_LINEAR_Z",
			}
		};

		return true;
	}

	bool BlitStage::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (m_ppGraphicCommandLists == nullptr)
		{
			if (!CreateCommandLists())
			{
				return false;
			}
		}

		return true;
	}

	void BlitStage::UpdateTextureResource(
		const String& resourceName,
		const TextureView* const* ppPerImageTextureViews,
		const TextureView* const* ppPerSubImageTextureViews,
		const Sampler* const* ppPerImageSamplers,
		uint32 imageCount,
		uint32 subImageCount,
		bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		Texture* pTexture = ppPerImageTextureViews[0]->GetDesc().pTexture;

		for (BlitDescription& blitDescription : m_BlitDescriptions)
		{
			if (resourceName == blitDescription.SrcName)
				blitDescription.pSrcTexture = pTexture;

			if (resourceName == blitDescription.DstName)
				blitDescription.pDstTexture = pTexture;
		}
	}

	void BlitStage::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
		if (sleeping)
			return;

		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		CommandList* pCommandList = m_ppGraphicCommandLists[modFrameIndex];

		pCommandList->Begin(nullptr);

		for (BlitDescription& blitDescription : m_BlitDescriptions)
		{
			if (blitDescription.pSrcTexture != nullptr && blitDescription.pDstTexture != nullptr)
			{
				pCommandList->BlitTexture(
					blitDescription.pSrcTexture,
					ETextureState::TEXTURE_STATE_GENERAL,
					blitDescription.pDstTexture,
					ETextureState::TEXTURE_STATE_GENERAL,
					EFilterType::FILTER_TYPE_NEAREST);
			}
		}

		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}

	bool BlitStage::CreateCommandLists()
	{
		m_ppGraphicCommandAllocators = DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppGraphicCommandLists = DBG_NEW CommandList*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppGraphicCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Blit Stage Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppGraphicCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName		= "Blit Stage Graphics Command List " + std::to_string(b);
			commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppGraphicCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppGraphicCommandAllocators[b], &commandListDesc);

			if (!m_ppGraphicCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}
}