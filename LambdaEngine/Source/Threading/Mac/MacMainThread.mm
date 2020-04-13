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
            : m_RunLoop(runLoop),
            m_RunLoopMode(runLoopMode),
            m_BlockLock(),
            m_Blocks()
        {
            CFRunLoopSourceContext sourceContext = { };
            sourceContext.info              = (void*)this;
            sourceContext.version           = 0;
            sourceContext.equal             = CFEqual;
            //sourceContext.hash              = CFHash;
            //sourceContext.retain            = CFRetain; //TODO: Investigate crash?
            sourceContext.release           = CFRelease;
            sourceContext.copyDescription   = CFCopyDescription;
            sourceContext.perform           = MacRunLoopSource::Perform;
            sourceContext.schedule          = MacRunLoopSource::Schedule;
            sourceContext.cancel            = MacRunLoopSource::Cancel;
            
            m_Source = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
            
            ASSERT(m_Source != nullptr);
            
            CFRunLoopAddSource(m_RunLoop, m_Source, m_RunLoopMode);
        }
        
        ~MacRunLoopSource()
        {
            CFRunLoopRemoveSource(m_RunLoop, m_Source, m_RunLoopMode);
            CFRelease(m_Source);
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
        
        static void Cancel(void* pInfo, CFRunLoopRef runLoop, CFStringRef mode)
        {
            NSLog(@"Cancel");
        }
        
        static void Schedule(void* pInfo, CFRunLoopRef runLoop, CFStringRef mode)
        {
            NSLog(@"Schedule");
        }
        
    private:
        CFRunLoopRef        m_RunLoop       = nullptr;
        CFRunLoopSourceRef  m_Source        = nullptr;
        CFStringRef         m_RunLoopMode   = nullptr;
        
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
            // If already on mainthread, execute block here
            block();
        }
        else
        {
            // Otherwise schedule block on main thread
            s_pMainThread->ScheduleBlock(block);
            s_pMainThread->WakeUp();
        }
    }
}

#endif
