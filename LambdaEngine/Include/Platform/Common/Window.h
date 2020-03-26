#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window
    {
    public:
        Window()    = default;
        ~Window()   = default;
        
        DECL_REMOVE_COPY(Window);
        DECL_REMOVE_MOVE(Window);
        
        virtual bool Init(uint32 width, uint32 height) = 0;
        virtual void Release() = 0;
        
        virtual void Show() = 0;
        
        virtual void* GetHandle() const = 0;
    };
}
