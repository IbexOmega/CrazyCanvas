#ifdef LAMBDA_PLATFORM_MACOS
#include "Containers/TArray.h"

#include <mutex>

#include "Threading/API/SpinLock.h"
#include "Threading/Mac/MacMainThread.h"

#include <Foundation/Foundation.h>

namespace LambdaEngine
{
    /*
     * Create definition for MacRunLoopSource, is not needed outside this compilation unit
     */

    class MacRunLoopSource
    {
    public:
        MacRunLoopSource(CFRunLoopRef runLoop, CFStringRef runLoopMode)
            : m_RunLoop(runLoop)
        {
            CFRunLoopSourceContext sourceContext = { };
            sourceContext.info      = (void*)this;
            sourceContext.version   = 0;
            sourceContext.perform   = MacRunLoopSource::Perform;
            sourceContext.schedule  = MacRunLoopSource::Schedule;
            sourceContext.cancel    = MacRunLoopSource::Cancel;
            
            m_Source = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
            
            ASSERT(m_Source != nullptr);
            
            CFRunLoopAddSource(m_RunLoop, m_Source, runLoopMode);
        }
        
        ~MacRunLoopSource()
        {
            CFRunLoopRemoveSource(m_RunLoop, m_Source, kCFRunLoopCommonModes);
            CFRelease(m_Source);
        }
        
        void ScheduleBlock(dispatch_block_t block)
        {
            std::scoped_lock<SpinLock> lock(m_BlockLock);
            m_Blocks.push_back(Block_copy(block));
            
            NSLog(@"ScheduleBlock");
            
            CFRunLoopSourceSignal(m_Source);
        }
        
        void Execute()
        {
            NSLog(@"Execute");
            
            // Copy blocks
            TArray<dispatch_block_t> blocksCopy;
            {
                std::scoped_lock<SpinLock> lock(m_BlockLock);
                blocksCopy.swap(m_Blocks);
            }
            
            // Execute all blocks
            for (dispatch_block_t block : blocksCopy)
            {
                block();
                Block_release(block);
            }
        }
        
        void WakeUp()
        {
            NSLog(@"WakeUp");
            CFRunLoopWakeUp(m_RunLoop);
        }
        
    private:
        static void Schedule(void* pInfo, CFRunLoopRef runLoop, CFStringRef mode)
        {
            NSLog(@"Schedule");
        }
        
        static void Cancel(void* pInfo, CFRunLoopRef runLoop, CFStringRef mode)
        {
            NSLog(@"Cancel");
        }
        
        static void Perform(void* pInfo)
        {
            NSLog(@"Perform");

            MacRunLoopSource* pRunLoopSource = reinterpret_cast<MacRunLoopSource*>(pInfo);
            if (pRunLoopSource)
            {
                pRunLoopSource->Execute();
            }
        }
        
    private:
        CFRunLoopRef        m_RunLoop   = nullptr;
        CFRunLoopSourceRef  m_Source    = nullptr;
        
        SpinLock m_BlockLock;
        TArray<dispatch_block_t> m_Blocks;
    };

    /*
     * MacMainThread
     */

    MacRunLoopSource* MacMainThread::s_pMainThread = nullptr;
    
    void MacMainThread::PreInit()
    {
        CFRunLoopRef mainLoop = CFRunLoopGetMain();
        s_pMainThread = DBG_NEW MacRunLoopSource(mainLoop, kCFRunLoopDefaultMode);
    }

    void MacMainThread::PostRelease()
    {
        SAFEDELETE(s_pMainThread);
    }

    void MacMainThread::MakeCall(dispatch_block_t block)
    {
        if ([NSThread isMainThread])
        {
            //If already on mainthread, execute block here
            block();
        }
        else
        {
            //Otherwise schedule it on the mainthread
            s_pMainThread->ScheduleBlock(block);
            s_pMainThread->WakeUp();
        }
    }
}

#endif
