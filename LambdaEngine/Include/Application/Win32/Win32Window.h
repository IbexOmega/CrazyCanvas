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

		bool Init(const WindowDesc* pDesc);

	public:
		// Window interface
		virtual void Show() 	override final;
		virtual void Close() 	override final;

		virtual void Minimize() override final;
        virtual void Maximize() override final;

		virtual bool IsActiveWindow() const override final;

		virtual void Restore() override final;

		virtual void ToggleFullscreen() override final;

		virtual void SetTitle(const String& title) override final;

		virtual void SetPosition(int32 x, int32 y)					override final;
		virtual void GetPosition(int32* pPosX, int32* pPosY) const	override final;

		virtual void SetSize(uint16 width, uint16 height) override final;

		virtual uint16	GetWidth()	const override final;
		virtual uint16	GetHeight()	const override final;
		virtual void* 	GetHandle()	const override final;

	private:
		HWND m_hWnd = 0;
	};
}

#endif