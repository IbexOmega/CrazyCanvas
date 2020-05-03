#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Application/API/Window.h"

#include "Windows.h"

#define WINDOW_CLASS L"MainWindowClass"

namespace LambdaEngine
{
	class Win32Window : public Window
	{
	public:
		Win32Window() = default;
		~Win32Window();

		// Window interface
		virtual bool Init(const char* pTitle, uint32 width, uint32 height) override final;

		virtual void Show() 	override final;
		virtual void Close() 	override final;

		virtual void Minimize() override final;
        virtual void Maximize() override final;

		virtual void SetTitle(const char* pTitle) override final;

		virtual uint16		GetWidth()	const override final;
		virtual uint16		GetHeight() const override final;
		virtual void* 		GetHandle() const override final;
		virtual const void* GetView() 	const override final;

	private:
		HWND m_hWnd = 0;
	};
}

#endif