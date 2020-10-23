#include "Threading/API/ThreadPool.h"
#include "Threading/API/PlatformThread.h"

#include "Log/Log.h"

// In case hardware_concurrency() returns 0, this is the default amount of threads the thread pool will start
#define MIN_THREADS 4u

namespace LambdaEngine
{
	TArray<std::thread> ThreadPool::s_Threads;
	std::queue<ThreadJob> ThreadPool::s_Jobs;
	std::condition_variable ThreadPool::s_JobsExist;

	// Pool of join resources to each thread job. Expanded upon need.
	TArray<JoinResources*> ThreadPool::s_JoinResources;
	TArray<uint32> ThreadPool::s_FreeJoinResourcesIndices;
	std::mutex ThreadPool::s_ScheduleLock;

	bool ThreadPool::s_TimeToTerminate = false;

	bool ThreadPool::Init()
	{
		// hardware_concurrency might return 0
		unsigned int hwConc = std::thread::hardware_concurrency();
		unsigned int threadCount = hwConc ? hwConc : MIN_THREADS;

		s_Threads.Reserve(threadCount);
		for (uint32 threadIdx = 0u; threadIdx < threadCount; threadIdx++)
		{
			std::thread& thread = s_Threads.EmplaceBack(std::thread(&ThreadPool::WaitForJob));
			PlatformThread::SetThreadName(PlatformThread::GetThreadHandle(thread), "ThreadPool" + std::to_string(threadIdx));
		}

		s_JoinResources.Resize(s_Threads.GetSize() * 2u);
		s_FreeJoinResourcesIndices.Resize(s_JoinResources.GetSize());

		for (uint32 threadHandleIdx = 0u; threadHandleIdx < s_JoinResources.GetSize(); threadHandleIdx++)
		{
			s_JoinResources[threadHandleIdx] = DBG_NEW JoinResources();
			s_JoinResources[threadHandleIdx]->FinishSignal = false;
			s_FreeJoinResourcesIndices[threadHandleIdx] = threadHandleIdx;
		}

		LOG_INFO("Started thread pool with %ld threads", threadCount);
		return true;
	}

	bool ThreadPool::Release()
	{
		JoinAll();

		s_ScheduleLock.lock();
		s_TimeToTerminate = true;
		s_JobsExist.notify_all();
		s_ScheduleLock.unlock();

		for (std::thread& thread : s_Threads)
		{
			thread.join();
		}

		for (JoinResources* pJoinResources : s_JoinResources)
		{
			delete pJoinResources;
		}

		return true;
	}

	uint32 ThreadPool::Execute(std::function<void()> job)
	{
		s_ScheduleLock.lock();
		uint32 joinResourceIdx = 0u;
		if (s_FreeJoinResourcesIndices.IsEmpty())
		{
			// Create new join resources
			joinResourceIdx = s_JoinResources.GetSize();
			s_JoinResources.EmplaceBack(DBG_NEW JoinResources());
		} else
		{
			joinResourceIdx = s_FreeJoinResourcesIndices.GetBack();
			s_FreeJoinResourcesIndices.PopBack();
		}

		s_JoinResources[joinResourceIdx]->FinishSignal = false;
		s_Jobs.push({ job, joinResourceIdx });
		s_JobsExist.notify_one();
		s_ScheduleLock.unlock();

		return joinResourceIdx;
	}

	void ThreadPool::ExecuteDetached(std::function<void()> job)
	{
		s_ScheduleLock.lock();
		s_Jobs.push({ job, UINT32_MAX });
		s_JobsExist.notify_one();
		s_ScheduleLock.unlock();
	}

	void ThreadPool::Join(uint32 joinResourcesIndex)
	{
		JoinResources* pJoinResources = s_JoinResources[joinResourcesIndex];
		std::unique_lock<std::mutex> uLock(pJoinResources->Mutex);
		pJoinResources->CondVar.wait(uLock, [pJoinResources]{ return pJoinResources->FinishSignal; });

		// Free the join resources
		s_ScheduleLock.lock();
		s_FreeJoinResourcesIndices.PushBack(joinResourcesIndex);
		s_ScheduleLock.unlock();
	}

	void ThreadPool::JoinAll()
	{
		// Wait for all jobs to finish
		std::unique_lock<std::mutex> uLock(s_ScheduleLock);
		s_JobsExist.wait(uLock, []{ return s_Jobs.empty() && s_FreeJoinResourcesIndices.GetSize() == s_JoinResources.GetSize(); });
	}

	void ThreadPool::WaitForJob()
	{
		std::unique_lock<std::mutex> uLock(s_ScheduleLock);
		while (true)
		{
			s_JobsExist.wait(uLock, []{ return !s_Jobs.empty() || s_TimeToTerminate; });

			if (!s_Jobs.empty())
			{
				ThreadJob threadJob = s_Jobs.front();
				s_Jobs.pop();

				s_ScheduleLock.unlock();
				threadJob.Function();
				s_ScheduleLock.lock();

				// Notify joining threads that the job is finished
				if (threadJob.JoinResourcesIndex != UINT32_MAX)
				{
					JoinResources* pJoinResources = s_JoinResources[threadJob.JoinResourcesIndex];

					pJoinResources->Mutex.lock();
					pJoinResources->FinishSignal = true;
					pJoinResources->CondVar.notify_all();
					pJoinResources->Mutex.unlock();
				}
			}

			if (s_TimeToTerminate)
			{
				break;
			}
		}
	}
}
