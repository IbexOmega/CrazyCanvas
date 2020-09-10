#pragma once

#include "Defines.h"

#include "Containers/TArray.h"

#include <condition_variable>
#include <functional>
#include <queue>

namespace LambdaEngine
{
	// Used to block threads calling join(), and used by working threads to notify when the job is finished
	struct JoinResources {
		std::mutex Mutex;
		std::condition_variable CondVar;
		bool FinishSignal;
	};

	struct ThreadJob {
		std::function<void()> Function;
		uint32 JoinResourcesIndex; // uint32_MAX is specified when join resources should not be used
	};

	class LAMBDA_API ThreadPool
	{
	public:
		DECL_STATIC_CLASS(ThreadPool);

		static bool Init();

		static bool Release();

		void Initialize();

		// Returns index to thread join resources. Calling join() on the returned index is required.
		static uint32 Execute(std::function<void()> job);

		// Schedules a job without any join resources attached. Calling joinAll() before the program exits is required.
		static void ExecuteDetached(std::function<void()> job);

		static void Join(uint32 joinResourcesIndex);
		static void JoinAll();

		static uint32 GetThreadCount() { return s_Threads.GetSize(); }

	private:
		// Infinite loop where threads wait for jobs
		static void WaitForJob();

	private:
		static TArray<std::thread> s_Threads;
		static std::queue<ThreadJob> s_Jobs;
		static std::condition_variable s_JobsExist;

		// Pool of join resources to each thread job. Expanded upon need.
		static TArray<JoinResources*> s_JoinResources;
		static TArray<uint32> s_FreeJoinResourcesIndices;
		static std::mutex s_ScheduleLock;

		// Signals when threads should stop looking for jobs in order to delete the thread pool
		static bool s_TimeToTerminate;
	};
}
