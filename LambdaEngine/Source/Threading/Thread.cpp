#include "Threading/Thread.h"
#include "Log/Log.h"

namespace LambdaEngine
{
	std::vector<Thread*> Thread::s_ThreadsToJoin;
	SpinLock Thread::s_Lock;

	Thread::Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished) :
		m_Func(func),
		m_FuncOnFinished(funcOnFinished)
	{
		m_Thread = std::move(std::thread(&Thread::Run, this));
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

	void Thread::Sleep(int32 milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	Thread* Thread::Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished)
	{
		return new Thread(func, funcOnFinished);
	}

	void Thread::Run()
	{
		m_Func();
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_ThreadsToJoin.push_back(this);
	}

	void Thread::Join()
	{
		if (!s_ThreadsToJoin.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (Thread* thread : s_ThreadsToJoin)
			{
				thread->m_Thread.join();
				thread->m_FuncOnFinished();
				delete thread;
			}
			s_ThreadsToJoin = std::vector<Thread*>();
		}
	}
}