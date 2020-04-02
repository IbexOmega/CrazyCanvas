#pragma once

#include "Defines.h"
#include "Types.h"
#include "SpinLock.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace LambdaEngine
{
	class LAMBDA_API Thread
	{
		friend class EngineLoop;

	public:
		~Thread();

		void Wait();
		void Notify();

		static void Sleep(int32 milliseconds);
		static Thread* Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

	private:
		Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

		void Run();
		static void Join();

	private:
		std::thread* m_pThread;
		std::function<void()> m_Func;
		std::function<void()> m_FuncOnFinished;
		std::condition_variable m_Condition;
		std::mutex m_Mutex;
		std::atomic_bool m_ShouldYeild;

		static SpinLock m_Lock;
		static std::vector<Thread*> m_ThreadsToJoin;
	};
}
