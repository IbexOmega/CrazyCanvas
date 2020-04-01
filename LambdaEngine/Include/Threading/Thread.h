#pragma once

#include "Defines.h"
#include <vector>
#include <thread>

namespace LambdaEngine
{
	class LAMBDA_API Thread
	{
		friend class EngineLoop;

	public:
		~Thread();

		static Thread* Create(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

	private:
		Thread(const std::function<void()>& func, const std::function<void()>& funcOnFinished);

		void Run();
		static void Join();

	private:
		std::thread* m_pThread;
		std::function<void()> m_Func;
		std::function<void()> m_FuncOnFinished;

		static std::vector<Thread*> m_ThreadsToJoin;
	};
}
