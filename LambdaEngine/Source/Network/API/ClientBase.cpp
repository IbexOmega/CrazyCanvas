#include "Network/API/ClientBase.h"
#include "Threading/Thread.h"
#include "Log/Log.h"

namespace LambdaEngine
{
	ClientBase::ClientBase() : 
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
		m_TransmitterQueueIndex(0)
	{
		
	}

	ClientBase::~ClientBase()
	{
		if (!m_Release)
			LOG_ERROR("[ClientBase]: Do not use delete on a Client. Use the Release() function!");
	}

	bool ClientBase::SendPacket(NetworkPacket* packet)
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

	bool ClientBase::SendPacketImmediately(NetworkPacket* packet)
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

	void ClientBase::Release()
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

	const std::string& ClientBase::GetAddress() const
	{
		return m_Address;
	}

	uint16 ClientBase::GetPort() const
	{
		return m_Port;
	}

	int32 ClientBase::GetBytesSent() const
	{
		return m_NrOfBytesTransmitted;
	}

	int32 ClientBase::GetBytesReceived() const
	{
		return m_NrOfBytesReceived;
	}

	int32 ClientBase::GetPacketsSent() const
	{
		return m_NrOfPacketsTransmitted;
	}

	int32 ClientBase::GetPacketsReceived() const
	{
		return m_NrOfPacketsReceived;
	}

	/*******************************************
	*				PROTECTED				   *
	********************************************/

	bool ClientBase::StartThreads()
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

	void ClientBase::TerminateThreads()
	{
		m_Run = false;
	}

	bool ClientBase::ThreadsHaveTerminated() const
	{
		return m_pThreadReceiver == nullptr && m_pThreadTransmitter == nullptr;
	}

	bool ClientBase::ThreadsAreRunning() const
	{
		return m_TransmitterStarted && m_ReceiverStarted;
	}

	bool ClientBase::ShouldTerminate() const
	{
		return !m_Run;
	}

	void ClientBase::SetAddressAndPort(const std::string& address, uint16 port)
	{
		m_Address = address;
		m_Port = port;
	}

	void ClientBase::RegisterBytesReceived(int32 bytes)
	{
		m_NrOfBytesReceived += bytes;
	}

	void ClientBase::RegisterPacketsReceived(int32 packets)
	{
		m_NrOfPacketsReceived += packets;
	}

	/*******************************************
	*					PRIVATE				   *
	********************************************/

	void ClientBase::ThreadTransmitter()
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

	void ClientBase::ThreadReceiver()
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

		if(!m_TransmitterEnded)
			m_pThreadTransmitter->Notify();
	}

	/*
	* Called right before delete m_pThreadTransmitter
	*/
	void ClientBase::ThreadTransmitterDeleted()
	{
		OnThreadTerminated();

		m_pThreadTransmitter = nullptr;

		if (m_Release)
			Release();
	}

	/*
	* Called right before delete m_pThreadReceiver
	*/
	void ClientBase::ThreadReceiverDeleted()
	{
		OnThreadTerminated();

		m_pThreadReceiver = nullptr;

		if(m_Release)
			Release();
	}

	void ClientBase::SwapPacketQueues()
	{
		std::scoped_lock<SpinLock> lock(m_LockPackets);
		m_TransmitterQueueIndex %= 2;
	}

	bool ClientBase::IsReadyToTransmitPackets() const
	{
		return !ShouldTerminate() && m_ReadyForStart;
	}

	bool ClientBase::TransmitPackets(std::queue<NetworkPacket*>* packets)
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

	void ClientBase::OnThreadTerminated()
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

	void ClientBase::DeletePackets(std::queue<NetworkPacket*>* packets)
	{
		while (!packets->empty())
		{
			NetworkPacket* packet = packets->front();
			packets->pop();
			DeletePacket(packet);
		}
	}

	void ClientBase::DeletePacket(NetworkPacket* packet)
	{
		if (packet->ShouldAutoDelete())
			delete packet;
	}
}