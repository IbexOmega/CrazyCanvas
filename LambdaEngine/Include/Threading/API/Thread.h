#pragma once
#include "Defines.h"
#include "Types.h"
#include "SpinLock.h"
#include "Containers/String.h"
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
		Thread(const String& name, const std::function<void()>& func, const std::function<void()>& funcOnFinished);

		void Run();

	public:
		static void Sleep(int32 milliseconds);
		static Thread* Create(const String& name, const std::function<void()>& func, const std::function<void()>& funcOnFinished);

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
		String m_Name;

	private:
		static SpinLock* s_Lock;
		static TArray<Thread*>* s_ThreadsToJoin;
		static std::set<Thread*>* s_Threads;
	};
}
