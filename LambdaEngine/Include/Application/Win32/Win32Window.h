#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Application/API/IWindow.h"

#include "Windows.h"

#define WINDOW_CLASS L"MainWindowClass"

namespace LambdaEngine
{
	class Win32Window : public IWindow
	{
	public:
		Win32Window() = default;
		~Win32Window();

		bool Init(const WindowDesc* pDesc);

		// Window interface
		virtual void Show() 	override final;
		virtual void Close() 	override final;

		virtual void Minimize() override final;
        virtual void Maximize() override final;

		virtual void Restore() override final;

		virtual void ToggleFullscreen() override final;

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