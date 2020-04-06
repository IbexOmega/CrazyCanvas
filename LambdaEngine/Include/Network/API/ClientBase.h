#pragma once

#include "Defines.h"
#include "Types.h"

#include "Threading/SpinLock.h"

#include "Time/API/Timestamp.h"

#include "NetworkPacket.h"

#include <string>
#include <atomic>
#include <queue>
#include <set>

namespace LambdaEngine
{
	class Thread;

	class LAMBDA_API ClientBase
	{
	public:
		ClientBase();
		virtual ~ClientBase();

		/*
		* Sends a packet
		*/
		bool SendPacket(NetworkPacket* packet);

		/*
		* Sends a packet Immediately using the current thread
		*/
		bool SendPacketImmediately(NetworkPacket* packet);

		/*
		* Release all the resouces used by the client and will be deleted when each thread has terminated.
		*/
		void Release();

		/*
		* return - The currently used inet address.
		*/
		const std::string& GetAddress() const;

		/*
		* return - The currently used port.
		*/
		uint16 GetPort() const;

		/*
		* return - The total number of bytes sent
		*/
		int32 GetBytesSent() const;

		/*
		* return - The total number of bytes received
		*/
		int32 GetBytesReceived() const;

		/*
		* return - The total number of packets sent
		*/
		int32 GetPacketsSent() const;

		/*
		* return - The total number of packets received
		*/
		int32 GetPacketsReceived() const;

	protected:
		virtual void OnTransmitterStarted() = 0;
		virtual void OnReceiverStarted() = 0;
		virtual void UpdateReceiver(NetworkPacket* packet) = 0;
		virtual void OnThreadsTerminated() = 0;
		virtual void OnReleaseRequested() = 0;
		virtual bool TransmitPacket(NetworkPacket* packet) = 0;

		bool StartThreads();
		void TerminateThreads();
		bool ThreadsHaveTerminated() const;
		bool ThreadsAreRunning() const;
		bool ShouldTerminate() const;
		void SetAddressAndPort(const std::string& address, uint16 port);
		void RegisterBytesReceived(int32 bytes);
		void RegisterPacketsReceived(int32 packets);

	private:
		void ThreadTransmitter();
		void ThreadReceiver();
		void ThreadTransmitterDeleted();
		void ThreadReceiverDeleted();
		bool IsReadyToTransmitPackets() const;
		void SwapPacketQueues();
		bool TransmitPackets(std::queue<NetworkPacket*>* packets);
		void OnThreadTerminated();

	private:
		static void DeletePackets(std::queue<NetworkPacket*>* packets);
		static void DeletePacket(NetworkPacket* packet);

	private:
		Thread* m_pThreadTransmitter;
		Thread* m_pThreadReceiver;

		SpinLock m_LockPackets;

		std::atomic_bool m_Run;
		std::atomic_bool m_ReadyForStart;
		std::atomic_bool m_TransmitterStarted;
		std::atomic_bool m_ReceiverStarted;
		std::atomic_bool m_TransmitterEnded;
		std::atomic_bool m_OtherThreadTerminated;
		std::atomic_bool m_Release;

		std::queue<NetworkPacket*> m_Packets[2];
		int8 m_TransmitterQueueIndex;

		std::string m_Address;
		uint16 m_Port;

		uint32 m_NrOfPacketsTransmitted;
		uint32 m_NrOfPacketsReceived;
		uint32 m_NrOfBytesTransmitted;
		uint32 m_NrOfBytesReceived;
	};
}
