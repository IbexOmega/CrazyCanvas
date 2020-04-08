#pragma once

#include "Defines.h"
#include "Types.h"

#include "Network/API/NetworkPacket.h"

#include "Threading/SpinLock.h"
#include "Threading/Thread.h"

#include "Time/API/Timestamp.h"

#include "Log/Log.h"

#include "Containers/String.h"
#include "Containers/TSet.h"


#include <atomic>
#include <queue>
#include <set>

namespace LambdaEngine
{
	template <typename IBase>
	class LAMBDA_API ClientBase : public IBase
	{
	public:
		ClientBase(bool serverSide) :
			m_NrOfBytesTransmitted(0),
			m_NrOfBytesReceived(0),
			m_NrOfPacketsTransmitted(0),
			m_NrOfPacketsReceived(0),
			m_pThreadTransmitter(nullptr),
			m_pThreadReceiver(nullptr),
			m_Run(false),
			m_ReadyForStart(false),
			m_TransmitterStarted(false),
			m_ReceiverStarted(false),
			m_TransmitterEnded(false),
			m_Release(false),
			m_OtherThreadTerminated(false),
			m_Port(0),
			m_TransmitterQueueIndex(0),
			m_ServerSide(serverSide)
		{

		}

		virtual ~ClientBase()
		{
			if (!m_Release)
				LOG_ERROR("[ClientBase]: Do not use delete on a Client. Use the Release() function!");
		}

		bool SendPacket(NetworkPacket* packet) override final
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (IsReadyToTransmitPackets())
			{
				m_Packets[m_TransmitterQueueIndex].push(packet);
				m_pThreadTransmitter->Notify();
				return true;
			}
			return false;
		}

		bool SendPacketImmediately(NetworkPacket* packet) override final
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (IsReadyToTransmitPackets())
			{
				packet->Pack();
				bool result = TransmitPacket(packet);
				if (result)
				{
					m_NrOfPacketsTransmitted++;
					m_NrOfBytesTransmitted += packet->GetSize();
				}
				else
				{
					TerminateThreads();
				}
				DeletePacket(packet);
				return result;
			}
			return false;
		}

		void Release() override
		{
			if (!m_Release)
			{
				m_Release = true;
				OnReleaseRequested();
				TerminateThreads();
			}

			if (ThreadsHaveTerminated())
			{
				delete this;
			}
		}

		bool IsServerSide() const override final
		{
			return m_ServerSide;
		}

		const std::string& GetAddress() const override final
		{
			return m_Address;
		}

		uint16 GetPort() const override final
		{
			return m_Port;
		}

		int32 GetBytesSent() const override final
		{
			return m_NrOfBytesTransmitted;
		}

		int32 GetBytesReceived() const override final
		{
			return m_NrOfBytesReceived;
		}
		
		int32 GetPacketsSent() const override final
		{
			return m_NrOfPacketsTransmitted;
		}

		int32 GetPacketsReceived() const override final
		{
			return m_NrOfPacketsReceived;
		}

	protected:
		virtual void OnTransmitterStarted() = 0;
		virtual void OnReceiverStarted() = 0;
		virtual void UpdateReceiver(NetworkPacket* packet) = 0;
		virtual void OnThreadsTerminated() = 0;
		virtual void OnReleaseRequested() = 0;
		virtual bool TransmitPacket(NetworkPacket* packet) = 0;

		bool StartThreads()
		{
			if (ThreadsHaveTerminated())
			{
				m_Run = true;
				m_ReadyForStart = false;
				m_TransmitterStarted = false;
				m_ReceiverStarted = false;
				m_TransmitterEnded = false;
				m_OtherThreadTerminated = false;
				m_Release = false;

				m_pThreadTransmitter = Thread::Create(
					std::bind(&ClientBase::ThreadTransmitter, this),
					std::bind(&ClientBase::ThreadTransmitterDeleted, this)
				);

				m_pThreadReceiver = Thread::Create(
					std::bind(&ClientBase::ThreadReceiver, this),
					std::bind(&ClientBase::ThreadReceiverDeleted, this)
				);
				m_ReadyForStart = true;
				return true;
			}

			return false;
		}

		void TerminateThreads()
		{
			m_Run = false;
		}

		bool ThreadsHaveTerminated() const
		{
			return m_pThreadReceiver == nullptr && m_pThreadTransmitter == nullptr;
		}

		bool ThreadsAreRunning() const
		{
			return m_TransmitterStarted && m_ReceiverStarted;
		}

		bool ShouldTerminate() const
		{
			return !m_Run;
		}

		void SetAddressAndPort(const std::string& address, uint16 port)
		{
			m_Address = address;
			m_Port = port;
		}

		void RegisterBytesReceived(int32 bytes)
		{
			m_NrOfBytesReceived += bytes;
		}

		void RegisterPacketsReceived(int32 packets)
		{
			m_NrOfPacketsReceived += packets;
		}

	private:
		void ThreadTransmitter()
		{
			while (!m_ReadyForStart) {}

			OnTransmitterStarted();
			m_TransmitterStarted = true;
			while (!m_ReceiverStarted) {}

			while (!ShouldTerminate())
			{
				std::queue<NetworkPacket*>* packets = &m_Packets[m_TransmitterQueueIndex];
				if (packets->empty())
					m_pThreadTransmitter->Wait();

				SwapPacketQueues();
				TransmitPackets(packets);
			}

			m_TransmitterEnded = true;
		}

		void ThreadReceiver()
		{
			while (!m_ReadyForStart) {}

			OnReceiverStarted();
			m_ReceiverStarted = true;
			while (!m_TransmitterStarted) {};

			NetworkPacket packet(EPacketType::PACKET_TYPE_UNDEFINED, false);
			while (!ShouldTerminate())
			{
				UpdateReceiver(&packet);
			}

			if (!m_TransmitterEnded)
				m_pThreadTransmitter->Notify();
		}

		/*
		* Called right before delete m_pThreadTransmitter
		*/
		void ThreadTransmitterDeleted()
		{
			OnThreadTerminated();

			m_pThreadTransmitter = nullptr;

			if (m_Release)
				Release();
		}

		/*
		* Called right before delete m_pThreadReceiver
		*/
		void ThreadReceiverDeleted()
		{
			OnThreadTerminated();

			m_pThreadReceiver = nullptr;

			if (m_Release)
				Release();
		}

		void SwapPacketQueues()
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			m_TransmitterQueueIndex %= 2;
		}

		bool IsReadyToTransmitPackets() const
		{
			return !ShouldTerminate() && m_ReadyForStart;
		}

		bool TransmitPackets(std::queue<NetworkPacket*>* packets)
		{
			while (!packets->empty())
			{
				NetworkPacket* packet = packets->front();
				packets->pop();

				if (!SendPacketImmediately(packet))
				{
					DeletePackets(packets);
					TerminateThreads();
					return false;
				}
			}
			return true;
		}

		void OnThreadTerminated()
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (m_OtherThreadTerminated)
			{
				OnThreadsTerminated();
				m_TransmitterStarted = false;
				m_ReceiverStarted = false;
				LOG_WARNING("Client Threads Terminated");
			}
			m_OtherThreadTerminated = true;
		}

	private:
		static void DeletePackets(std::queue<NetworkPacket*>* packets)
		{
			while (!packets->empty())
			{
				NetworkPacket* packet = packets->front();
				packets->pop();
				DeletePacket(packet);
			}
		}

		static void DeletePacket(NetworkPacket* packet)
		{
			if (packet->ShouldAutoDelete())
				delete packet;
		}

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
		bool m_ServerSide;

		uint32 m_NrOfPacketsTransmitted;
		uint32 m_NrOfPacketsReceived;
		uint32 m_NrOfBytesTransmitted;
		uint32 m_NrOfBytesReceived;
	};
}
