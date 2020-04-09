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
			m_ThreadsStarted(false),
			m_ThreadsStartedPre(false),
			m_TransmitterEnded(false),
			m_Release(false),
			m_OtherThreadTerminated(false),
			m_Port(0),
			m_TransmitterQueueIndex(0),
			m_ServerSide(serverSide)
		{
			m_pPackets[0] = new std::queue<NetworkPacket*>();
			m_pPackets[1] = new std::queue<NetworkPacket*>();
		}

		virtual ~ClientBase()
		{
			delete m_pPackets[0];
			delete m_pPackets[1];

			if (!m_Release)
				LOG_ERROR("[ClientBase]: Do not use delete on a Client. Use the Release() function!");
		}

		virtual bool SendPacket(NetworkPacket* packet, bool flush = false) override final
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (IsReadyToTransmitPackets())
			{
				m_pPackets[m_TransmitterQueueIndex]->push(packet);
				if (flush)
				{
					m_pThreadTransmitter->Notify();
				}
				return true;
			}
			return false;
		}

		virtual bool SendPacketImmediately(NetworkPacket* packet) override final
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

		virtual void Flush() override
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (m_pThreadTransmitter)
			{
				m_pThreadTransmitter->Notify();
			}
		}

		virtual void Release() override
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

		virtual bool IsServerSide() const override final
		{
			return m_ServerSide;
		}

		virtual const std::string& GetAddress() const override final
		{
			return m_Address;
		}

		virtual uint16 GetPort() const override final
		{
			return m_Port;
		}

		virtual int32 GetBytesSent() const override final
		{
			return m_NrOfBytesTransmitted;
		}

		virtual int32 GetBytesReceived() const override final
		{
			return m_NrOfBytesReceived;
		}
		
		virtual int32 GetPacketsSent() const override final
		{
			return m_NrOfPacketsTransmitted;
		}

		virtual int32 GetPacketsReceived() const override final
		{
			return m_NrOfPacketsReceived;
		}

	protected:
		virtual bool OnThreadsStarted() = 0;
		virtual bool OnThreadsStartedPost() = 0;
		virtual void UpdateReceiver(NetworkPacket* packet) = 0;
		virtual void OnThreadsTerminated() = 0;
		virtual void OnReleaseRequested() = 0;
		virtual bool TransmitPacket(NetworkPacket* packet) = 0;

		virtual bool StartThreads()
		{
			if (ThreadsHaveTerminated())
			{
				
			}

			return false;
		}

		virtual void TerminateThreads()
		{
			m_Run = false;
		}

		virtual bool ThreadsHaveTerminated() const
		{
			return m_pThreadReceiver == nullptr && m_pThreadTransmitter == nullptr;
		}

		virtual bool ThreadsAreRunning() const
		{
			return m_ThreadsStartedPre;
		}

		virtual bool ShouldTerminate() const
		{
			return !m_Run;
		}

		virtual void SetAddressAndPort(const std::string& address, uint16 port)
		{
			m_Address = address;
			m_Port = port;
		}

		virtual void RegisterBytesReceived(int32 bytes)
		{
			m_NrOfBytesReceived += bytes;
		}

		virtual void RegisterPacketsReceived(int32 packets)
		{
			m_NrOfPacketsReceived += packets;
		}

	private:
		virtual void ThreadTransmitter()
		{
			while (!m_ReadyForStart) {}

			if (!OnThreadsStarted())
			{
				TerminateThreads();
			}
			else
			{
				m_ThreadsStartedPre = true;
				if (!OnThreadsStartedPost())
					TerminateThreads();
			}
			m_ThreadsStarted = true;

			while (!ShouldTerminate())
			{
				std::queue<NetworkPacket*>* packets = m_pPackets[m_TransmitterQueueIndex];
				if (packets->empty())
					m_pThreadTransmitter->Wait();

				SwapPacketQueues();
				TransmitPackets(packets);
			}

			m_TransmitterEnded = true;
		}

		virtual void ThreadReceiver()
		{
			while (!m_ThreadsStarted && !ShouldTerminate()) {}

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
		virtual void ThreadTransmitterDeleted()
		{
			OnThreadTerminated();

			m_pThreadTransmitter = nullptr;

			if (m_Release)
				Release();
		}

		/*
		* Called right before delete m_pThreadReceiver
		*/
		virtual void ThreadReceiverDeleted()
		{
			OnThreadTerminated();

			m_pThreadReceiver = nullptr;

			if (m_Release)
				Release();
		}

		virtual void SwapPacketQueues()
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			m_TransmitterQueueIndex = m_TransmitterQueueIndex % 2;
		}

		virtual bool TransmitPackets(std::queue<NetworkPacket*>* packets)
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

		virtual void OnThreadTerminated()
		{
			std::scoped_lock<SpinLock> lock(m_LockPackets);
			if (m_OtherThreadTerminated)
			{
				OnThreadsTerminated();
				m_ThreadsStarted = false;
				LOG_WARNING("Client Threads Terminated");
			}
			m_OtherThreadTerminated = true;
		}

		virtual bool IsReadyToTransmitPackets() const
		{
			return !ShouldTerminate() && m_ThreadsStartedPre;
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
		std::atomic_bool m_ThreadsStartedPre;
		std::atomic_bool m_ThreadsStarted;
		std::atomic_bool m_TransmitterEnded;
		std::atomic_bool m_OtherThreadTerminated;
		std::atomic_bool m_Release;

		std::queue<NetworkPacket*>* m_pPackets[2];
		std::atomic_int m_TransmitterQueueIndex;

		std::string m_Address;
		uint16 m_Port;
		bool m_ServerSide;

		uint32 m_NrOfPacketsTransmitted;
		uint32 m_NrOfPacketsReceived;
		uint32 m_NrOfBytesTransmitted;
		uint32 m_NrOfBytesReceived;
	};
}
