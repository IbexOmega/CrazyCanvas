#include "Thread.h"

namespace LambdaEngine
{
	std::vector<Thread*> Thread::m_ThreadsToJoin;

	Thread::Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished) :
		m_Func(func),
		m_FuncOnFinished(funcOnFinished)
	{
		m_pThread = new std::thread(&Thread::Run, this);
	}

	Thread::~Thread()
	{

	}

	Thread* Thread::Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished)
	{
		return new Thread(func, funcOnFinished);
	}

	void Thread::Run()
	{
		m_Func();
		//Implement locking here
		m_ThreadsToJoin.push_back(this); 
	}

	void Thread::Join()
	{
		//Implement locking here
		for (Thread* thread : m_ThreadsToJoin)
		{
			thread->m_pThread->join();
			thread->m_FuncOnFinished();
			delete thread;
		}
		m_ThreadsToJoin.clear();
	}
}