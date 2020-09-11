#pragma once

#include "LambdaEngine.h"

#include "Threading/API/SpinLock.h"

#include "Networking/API/NetworkSegment.h"

#include <atomic>

namespace LambdaEngine
{
	class Thread;

	class LAMBDA_API NetWorker
	{
	public:
		DECL_UNIQUE_CLASS(NetWorker);
		NetWorker();
		virtual ~NetWorker();

		void Flush();

	protected:
		virtual bool OnThreadsStarted() = 0;
		virtual void RunTransmitter() = 0;
		virtual void RunReceiver() = 0;
		virtual void OnThreadsTerminated() = 0;
		virtual void OnTerminationRequested() = 0;
		virtual void OnReleaseRequested() = 0;

		bool StartThreads();
		void TerminateThreads();
		bool ThreadsAreRunning() const;
		bool ThreadsHasTerminated() const;
		bool ShouldTerminate() const;
		void YieldTransmitter();
		void TerminateAndRelease();

	private:
		void ThreadTransmitter();
		void ThreadReceiver();
		void ThreadTransmitterDeleted();
		void ThreadReceiverDeleted();
		void ThreadsDeleted();

	protected:
		char m_pReceiveBuffer[UINT16_MAX];

	private:
		Thread* m_pThreadTransmitter;
		Thread* m_pThreadReceiver;

		SpinLock m_Lock;

		std::atomic_bool m_Run;
		std::atomic_bool m_ThreadsStarted;
		std::atomic_bool m_Initiated;
		std::atomic_bool m_ReceiverStopped;
		std::atomic_bool m_ThreadsTerminated;
		std::atomic_bool m_Release;

	private:
		static SpinLock s_LockStatic;
	};
}