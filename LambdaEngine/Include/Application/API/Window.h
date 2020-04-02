#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window
    {
    public:       
		DECL_ABSTRACT_CLASS(Window);
        
        virtual bool Init(const char* pTitle, uint32 width, uint32 height)  = 0;
        
        virtual void Show() = 0;
        
        virtual void*       GetHandle() const = 0;
        virtual const void* GetView()   const = 0;
    };
}
