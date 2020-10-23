#include "Threading/API/Thread.h"
#include "Threading/API/PlatformThread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	TArray<Thread*>* Thread::s_ThreadsToJoin;
	std::set<Thread*>* Thread::s_Threads;
	SpinLock* Thread::s_Lock;

	Thread::Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished) :
		m_Func(func),
		m_FuncOnFinished(funcOnFinished)
	{
		std::scoped_lock<SpinLock> lock(*s_Lock);
		s_Threads->insert(this);
		m_Thread = std::thread(&Thread::Run, this);
	}

	Thread::~Thread()
	{
	}

	void Thread::Wait()
	{
		if (m_Thread.get_id() == std::this_thread::get_id())
		{
			m_ShouldYeild = true;
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Condition.wait(lock, [this]{ return !m_ShouldYeild.load(); });
		}
	}

	void Thread::Notify()
	{
		m_ShouldYeild = false;
		m_Condition.notify_one();
	}

	void Thread::SetName(const String& name)
	{
		PlatformThread::SetThreadName(PlatformThread::GetThreadHandle(m_Thread), name);
	}

	void Thread::Sleep(int32 milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	Thread* Thread::Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished)
	{
		return DBG_NEW Thread(func, funcOnFinished);
	}

	void Thread::Run()
	{
		m_Func();
		std::scoped_lock<SpinLock> lock(*s_Lock);
		s_ThreadsToJoin->PushBack(this);
	}

	void Thread::Init()
	{
		s_Lock = DBG_NEW SpinLock();
		s_Threads = DBG_NEW std::set<Thread*>();
		s_ThreadsToJoin = DBG_NEW TArray<Thread*>();
	}

	void Thread::Join()
	{
		if (!s_ThreadsToJoin->IsEmpty())
		{
			std::scoped_lock<SpinLock> lock(*s_Lock);
			for (Thread* thread : *s_ThreadsToJoin)
			{
				thread->m_Thread.join();
				if(thread->m_FuncOnFinished)
					thread->m_FuncOnFinished();
				s_Threads->erase(thread);
				delete thread;
			}
			s_ThreadsToJoin->Clear();
		}
	}

	void Thread::Release()
	{
		while (!s_Threads->empty())
			Thread::Join();

		delete s_Lock;
		delete s_Threads;
		delete s_ThreadsToJoin;
	}
}
