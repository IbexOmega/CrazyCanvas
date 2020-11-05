#include "Networking/API/NetWorker.h"

#include "Log/Log.h"

#include "Threading/API/Thread.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	SpinLock NetWorker::s_LockStatic;
	std::atomic_int NetWorker::s_Instances = 0;
	THashTable<NetWorker*, uint8> NetWorker::s_NetworkersToDelete;

	NetWorker::NetWorker() : 
		m_pThreadReceiver(nullptr),
		m_pThreadTransmitter(nullptr),
		m_Run(false),
		m_ThreadsStarted(false),
		m_ReceiverStopped(false),
		m_Initiated(false),
		m_ThreadsTerminated(true),
		m_Release(false),
		m_pReceiveBuffer()
	{
		s_Instances++;
	}

	NetWorker::~NetWorker()
	{
		if (!m_Release)
			LOG_ERROR("[NetWorker]: Do not use delete on a NetWorker object. Use the Release() function!");

		s_Instances--;
	}

	void NetWorker::Flush()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pThreadTransmitter)
			m_pThreadTransmitter->Notify();
	}

	void NetWorker::TerminateAndRelease(const std::string& reason)
	{
		std::scoped_lock<SpinLock> lock(s_LockStatic);
		if (!m_Release)
		{
			m_Release = true;
			OnReleaseRequested(reason);
			TerminateThreads(reason);
		}

		if (m_ThreadsTerminated)
		{
			s_NetworkersToDelete.insert(std::make_pair(this, (uint8)5));
		}
	}

	bool NetWorker::TerminateThreads(const std::string& reason)
	{
		if (m_Run)
		{
			m_Run = false;
			OnTerminationRequested(reason);
			Flush();
			return true;
		}
		return false;
	}

	bool NetWorker::ThreadsAreRunning() const
	{
		return m_pThreadReceiver != nullptr && m_pThreadTransmitter != nullptr;
	}

	bool NetWorker::ThreadsHasTerminated() const
	{
		return m_ThreadsTerminated;
	}

	bool NetWorker::ShouldTerminate() const
	{
		return !m_Run;
	}

	void NetWorker::YieldTransmitter()
	{
		if (m_pThreadTransmitter)
			m_pThreadTransmitter->Wait();
	}

	bool NetWorker::StartThreads()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (!ThreadsAreRunning() && m_ThreadsTerminated)
		{
			m_Run = true;
			m_ThreadsStarted = false;
			m_Initiated = false;
			m_ThreadsTerminated = false;

			m_pThreadTransmitter = Thread::Create(
				std::bind_front(&NetWorker::ThreadTransmitter, this),
				std::bind_front(&NetWorker::ThreadTransmitterDeleted, this)
			);

			m_pThreadReceiver = Thread::Create(
				std::bind_front(&NetWorker::ThreadReceiver, this),
				std::bind_front(&NetWorker::ThreadReceiverDeleted, this)
			);
			m_ThreadsStarted = true;
			return true;
		}
		return false;
	}

	void NetWorker::ThreadTransmitter()
	{
		while (!m_ThreadsStarted);

		std::string reason;
		if (!OnThreadsStarted(reason))
			TerminateThreads(reason);

		m_Initiated = true;

		RunTransmitter();

		while (!m_ReceiverStopped);
	}

	void NetWorker::ThreadReceiver()
	{
		while (!m_Initiated);

		RunReceiver();

		Flush();
		m_ReceiverStopped = true;
	}

	void NetWorker::ThreadTransmitterDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			m_pThreadTransmitter = nullptr;
		}

		if (!m_pThreadReceiver)
			ThreadsDeleted();
	}

	void NetWorker::ThreadReceiverDeleted()
	{
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			m_pThreadReceiver = nullptr;
		}

		if (!m_pThreadTransmitter)
			ThreadsDeleted();
	}

	void NetWorker::ThreadsDeleted()
	{
		OnThreadsTerminated();
		m_ThreadsTerminated = true;

		if (m_Release)
			TerminateAndRelease("All Threads Terminated");
	}

	void NetWorker::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_NetworkersToDelete.empty())
		{
			std::scoped_lock<SpinLock> lock(s_LockStatic);
			TArray<NetWorker*> networkersToDelete;
			for (auto& pair : s_NetworkersToDelete)
			{
				if (--pair.second == 0)
					networkersToDelete.PushBack(pair.first);
			}

			for (NetWorker* pNetworker : networkersToDelete)
			{
				s_NetworkersToDelete.erase(pNetworker);
				delete pNetworker;
			}
		}
	}

	void NetWorker::ReleaseStatic()
	{
		while (s_Instances > 0)
			FixedTickStatic(EngineLoop::GetFixedTimestep());
	}
}