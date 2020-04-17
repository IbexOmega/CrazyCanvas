#pragma once

#include "LambdaEngine.h"

#include "Threading/API/SpinLock.h"

#include "Networking/API/NetworkPacket.h"

#include <atomic>

namespace LambdaEngine
{
	class Thread;

	class LAMBDA_API NetWorker
	{
	public:
		NetWorker();
		virtual ~NetWorker();

		void Flush();
		void Release();

	protected:
		virtual bool OnThreadsStarted() = 0;
		virtual void RunTranmitter() = 0;
		virtual void RunReceiver() = 0;
		virtual void OnThreadsTurminated() = 0;
		virtual void OnTerminationRequested() = 0;
		virtual void OnReleaseRequested() = 0;

		bool StartThreads();
		void TerminateThreads();
		bool ThreadsAreRunning() const;
		bool ShouldTerminate() const;
		void YieldTransmitter();

	private:
		void ThreadTransmitter();
		void ThreadReceiver();
		void ThreadTransmitterDeleted();
		void ThreadReceiverDeleted();
		void ThreadsDeleted();

	protected:
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
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
	};
}