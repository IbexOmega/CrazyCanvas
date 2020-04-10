#pragma once
#include "Containers/String.h"

#include "Threading/API/SpinLock.h"

#include "Time/API/Timestamp.h"

#include "Defines.h"
#include "Types.h"

#include <set>

#define TRANSMIT_RATE_PPS 30
#define TRANSMIT_DELAY 1000000000 / TRANSMIT_RATE_PPS

namespace LambdaEngine
{
	class ISocketUDP;
	class Thread;

	class LAMBDA_API GameServerBase
	{
		friend class NetworkUtils;

	public:
		GameServerBase();
		virtual ~GameServerBase();

		bool Start(uint16 port);
		void Stop();
		void Release();
		bool IsRunning() const;
		const std::string& GetAddress() const;
		uint16 GetPort() const;

	protected:
		virtual void RunTranmitter() = 0;
		virtual void RunReceiver() = 0;
		virtual void OnThreadsTurminated() = 0;

		void TerminateThreads();
		bool ThreadsAreRunning() const;
		bool ShouldTerminate() const;

	private:
		void StartThreads();
		void ThreadTransmitter();
		void ThreadReceiver();
		void ThreadTransmitterDeleted();
		void ThreadReceiverDeleted();
		void ThreadsDeleted();
		bool SetupSocket();
		void FlushPackets();

	private:
		static void InitStatic();
		static void TickStatic(Timestamp dt);
		static void ReleaseStatic();

	private:
		ISocketUDP* m_pSocket;

		Thread* m_pThreadTransmitter;
		Thread* m_pThreadReceiver;

		SpinLock m_LockStart;
		SpinLock m_LockRelease;
		SpinLock m_LockSocket;
		SpinLock m_LockThread;

		std::atomic_bool m_Run;
		std::atomic_bool m_ThreadsStarted;
		std::atomic_bool m_SocketCreated;
		std::atomic_bool m_ReceiverStopped;
		std::atomic_bool m_ThreadsTerminated;
		std::atomic_bool m_Release;

		std::string m_Address;
		uint16 m_Port;

	private:
		static std::set<GameServerBase*>* s_Instances;
		static SpinLock* s_LockInstances;
		static int64 s_TimerTransmit;
	};
}
