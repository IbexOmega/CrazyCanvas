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
		Win32Window()	= default;
		~Win32Window();

		virtual bool Init(const char* pTitle, uint32 width, uint32 height) override;

		virtual void SetTitle(const char* pTitle) override;

		virtual void Show() override;

		FORCEINLINE virtual void* GetHandle() const override final
		{ 
			return (void*)m_hWnd; 
		}

		FORCEINLINE virtual const void* GetView() const override final
        {
            return nullptr;
        }

	private:
		HWND m_hWnd = 0;
	};
}

#endif