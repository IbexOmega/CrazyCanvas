#pragma once
#include "DeviceChild.h"
#include "CommandQueue.h"

#include "Application/API/Window.h"

#include "Core/Ref.h"

namespace LambdaEngine
{
	class Texture;
	
	struct SwapChainDesc
	{
		String				DebugName		= "";
		TSharedRef<Window>			Window			= nullptr;
		TSharedRef<CommandQueue>	Queue			= nullptr;
		EFormat				Format			= EFormat::FORMAT_NONE;
		uint32				Width			= 0;
		uint32				Height			= 0;
		uint32				BufferCount		= 0;
		uint32				SampleCount		= 0;
		bool				VerticalSync	= true;
	};

	class SwapChain : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(SwapChain);
		
		/*
		* Resizes the texturebuffers of the swapchain. This function should be externally syncronized.
		* This means that the caller has to make sure that no texture resources are in use when this function
		* is called.
		*	width	- The new witdh of the buffers
		*	height	- The new height of the buffers
		*	return	- Returns true if resizing is successfull or if the size is the same 
		*				as the current one. 
		*/
		virtual bool ResizeBuffers(uint32 width, uint32 height) = 0;
		
		/*
		* Swaps the backbuffers so that the current buffer gets displayed to the specified window
		*/
		virtual bool Present() = 0;

		/*
		* Returns a pointer to the window that the swapchain will present to during a call to present.
		*	return - Pointer to the window specified when the swapchain were created.
		*/
		virtual const Window* GetWindow() const
		{
			return m_Desc.Window.Get();
		}

		/*
		* Returns the buffer specified as parameter. Caller is responsible for calling release on the
		* resulting pointer. All references to must be released before calling Release or ResizeBuffers.
		*	bufferIndex	- The index of the desired buffer. Must be between 0 and (BufferCount - 1).
		*	return		- Returns a pointer to the desired texturebuffer
		*/
		virtual Texture*		GetBuffer(uint32 bufferIndex)		= 0;
		virtual const Texture*	GetBuffer(uint32 bufferIndex) const = 0;

		/*
		* Returns the CommandQueue that were used to create this swapchain. Caller is responsible for 
		* calling Release.
		*	return - On success a valid pointer is returned, otherwise nullptr
		*/
		virtual CommandQueue* GetCommandQueue()
		{
			return m_Desc.Queue.GetAndAddRef();
		}

		/*
		* Returns the index of the current backbuffer. A number between 0 and (BufferCount-1).
		*	return - Returns the backbuffer index
		*/
		virtual uint64 GetCurrentBackBufferIndex() const = 0;

		virtual SwapChainDesc GetDesc() const
		{
			return m_Desc;
		}

	protected:
		SwapChainDesc m_Desc;
	};
}
