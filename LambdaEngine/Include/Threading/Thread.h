#pragma once
#include "Defines.h"
#include "Types.h"
#include "SpinLock.h"
#include <set>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class LAMBDA_API Thread
	{
		friend class EngineLoop;

	public:
		~Thread();

		void Wait();
		void Notify();

	private:
		Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

		void Run();

	public:
		static void Sleep(int32 milliseconds);
		static Thread* Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

	private:
		static void Init();
		static void Join();
		static void Release();

	private:
		std::thread m_Thread;
		std::function<void()> m_Func;
		std::function<void()> m_FuncOnFinished;
		std::condition_variable m_Condition;
		std::mutex m_Mutex;
		std::atomic_bool m_ShouldYeild;

	private:
		static SpinLock* s_Lock;
		static std::vector<Thread*>* s_ThreadsToJoin;
		static std::set<Thread*>* s_Threads;
	};
}
