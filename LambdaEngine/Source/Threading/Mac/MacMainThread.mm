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
        MacRunLoopSource(CFRunLoopRef runLoop, NSString* runLoopMode)
            : m_RunLoop(runLoop),
            m_RunLoopMode(runLoopMode),
            m_BlockLock(),
            m_Blocks()
        {
            CFRunLoopSourceContext sourceContext = { };
            sourceContext.info    = (void*)this;
            sourceContext.version = 0;
            sourceContext.perform = MacRunLoopSource::Perform;
            
            m_Source = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
            
            VALIDATE(m_Source != nullptr);
            
            CFStringRef cfRunLoopMode = (CFStringRef)m_RunLoopMode;
            CFRunLoopAddSource(m_RunLoop, m_Source, cfRunLoopMode);
            CFRelease(m_Source);
        }
        
        ~MacRunLoopSource()
        {
        }
        
        void ScheduleBlock(dispatch_block_t block)
        {
            dispatch_block_t copyBlock = Block_copy(block);

            // Add block and signal source to perform next runloop iteration
            {
                std::scoped_lock<SpinLock> lock(m_BlockLock);
                m_Blocks.push_back(copyBlock);
            }
            
            CFRunLoopSourceSignal(m_Source);
        }
        
        void Execute()
        {
            // Copy blocks
            TArray<dispatch_block_t> copiedBlocks;
            {
                std::scoped_lock<SpinLock> lock(m_BlockLock);
                
                copiedBlocks = TArray<dispatch_block_t>(m_Blocks);
                m_Blocks.clear();
            }
            
            // Execute all blocks
            for (dispatch_block_t block : copiedBlocks)
            {
                block();
                Block_release(block);
            }
        }
        
        void RunInMode(CFRunLoopMode runMode)
        {
            CFRunLoopRunInMode(runMode, 0, true);
        }
        
        void WakeUp()
        {
            CFRunLoopWakeUp(m_RunLoop);
        }
        
    private:
        static void Perform(void* pInfo)
        {
            MacRunLoopSource* pRunLoopSource = reinterpret_cast<MacRunLoopSource*>(pInfo);
            if (pRunLoopSource)
            {
                pRunLoopSource->Execute();
            }
        }
        
    private:
        CFRunLoopRef        m_RunLoop       = nullptr;
        CFRunLoopSourceRef  m_Source        = nullptr;
        NSString*           m_RunLoopMode   = nullptr;
        
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
        s_pMainThread = DBG_NEW MacRunLoopSource(mainLoop, NSDefaultRunLoopMode);
    }

    void MacMainThread::PostRelease()
    {
        SAFEDELETE(s_pMainThread);
    }

    void MacMainThread::Tick()
    {
        VALIDATE(s_pMainThread != nullptr);
        s_pMainThread->RunInMode((CFRunLoopMode)NSDefaultRunLoopMode);
    }

    void MacMainThread::MakeCall(dispatch_block_t block, bool waitUntilFinished)
    {
        dispatch_block_t copiedBlock = Block_copy(block);
        
        if ([NSThread isMainThread])
        {
            // If already on mainthread, execute block here
            copiedBlock();
        }
        else
        {
            // Otherwise schedule block on main thread
            VALIDATE(s_pMainThread != nullptr);

            if (waitUntilFinished)
            {
                dispatch_semaphore_t    waitSemaphore   = dispatch_semaphore_create(0);
                dispatch_block_t        waitableBlock   = Block_copy(^{ copiedBlock(); dispatch_semaphore_signal(waitSemaphore); });
                
                s_pMainThread->ScheduleBlock(waitableBlock);
                s_pMainThread->RunInMode((CFStringRef)NSDefaultRunLoopMode);
                
                dispatch_semaphore_wait(waitSemaphore, DISPATCH_TIME_FOREVER);
                Block_release(waitableBlock);
            }
            else
            {
                s_pMainThread->ScheduleBlock(copiedBlock);
                s_pMainThread->WakeUp();
            }
        }
        
        Block_release(copiedBlock);
    }
}

#endif
