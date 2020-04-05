#include "ClientTCP2.h"
#include "PlatformSocketFactory.h"
#include "Log/Log.h"

namespace LambdaEngine
{
	NetworkPacket ClientTCP2::s_PacketPing(PACKET_TYPE_PING, false);
	std::set<ClientTCP2*>* ClientTCP2::s_Clients = nullptr;
	SpinLock* ClientTCP2::s_LockClients = nullptr;

	ClientTCP2::ClientTCP2(IClientTCPHandler* handler) : ClientTCP2(handler, nullptr)
	{
		
	}

	ClientTCP2::ClientTCP2(IClientTCPHandler* handler, ISocketTCP* socket) : 
		m_pSocket(socket),
		m_pHandler(handler),
		m_ServerSide(m_pSocket != nullptr), 
		m_TimerReceived(0),
		m_TimerTransmit(0),
		m_NrOfPingTransmitted(0),
		m_NrOfPingReceived(0)
	{
		if (IsServerSide())
		{
			StartThreads();
		}

		std::scoped_lock<SpinLock> lock(*s_LockClients);
		s_Clients->insert(this);
	}

	ClientTCP2::~ClientTCP2()
	{
		std::scoped_lock<SpinLock> lock(*s_LockClients);
		s_Clients->erase(this);
		LOG_WARNING("~ClientTCP2()");
	}

	bool ClientTCP2::Connect(const std::string& address, uint16 port)
	{
		std::scoped_lock<SpinLock> lock(m_LockStart);
		if (ThreadsHaveTerminated())
		{
			SetAddressAndPort(address, port);

			if (StartThreads())
			{
				return true;
			}
		}
		return false;
	}

	void ClientTCP2::Disconnect()
	{
		if (!ThreadsHaveTerminated())
		{
			TerminateThreads();
			std::scoped_lock<SpinLock> lock(m_LockStart);
			if(m_pSocket)
				m_pSocket->Close();
		}
	}

	bool ClientTCP2::IsServerSide() const
	{
		return m_ServerSide;
	}

	/*******************************************
	*				PROTECTED				   *
	********************************************/

	/*
	* Called by the Transmitter Thread when it starts
	*/
	void ClientTCP2::OnTransmitterStarted()
	{
		if (!IsServerSide())
		{
			m_pSocket = CreateSocket(GetAddress(), GetPort());
			if (!m_pSocket)
			{
				if (m_pHandler)
				{
					m_pHandler->OnClientFailedConnecting(this);
				}
				
				TerminateThreads();
				return;
			}
		}

		m_pSocket->DisableNaglesAlgorithm();
		ResetReceiveTimer();
		ResetTransmitTimer();

		m_pHandler->OnClientConnected(this);
	}

	/*
	* Called by the Receiver Thread when it starts
	*/
	void ClientTCP2::OnReceiverStarted()
	{
		
	}

	/*
	* Called Repeatedly by the Receiver Thread
	*/
	void ClientTCP2::UpdateReceiver(NetworkPacket* packet)
	{
		if (ReceivePacket(packet))
		{
			RegisterPacketsReceived(1);
			ResetReceiveTimer();
			HandlePacket(packet);
		}
		else
		{
			TerminateThreads();
		}
	}

	/*
	* Called by the Transmitter or Reciever Thread when both has terminated
	*/
	void ClientTCP2::OnThreadsTerminated()
	{
		LOG_MESSAGE("OnThreadsTerminated()");
		std::scoped_lock<SpinLock> lock(m_LockStart);
		delete m_pSocket;
		m_pSocket = nullptr;

		if (m_pHandler)
		{
			m_pHandler->OnClientDisconnected(this);
			m_pHandler = nullptr;
		}
	}

	void ClientTCP2::OnReleaseRequested()
	{
		Disconnect();
		std::scoped_lock<SpinLock> lock(m_LockStart);
		m_pHandler = nullptr;
	}

	bool ClientTCP2::TransmitPacket(NetworkPacket* packet)
	{
		char* buffer = packet->GetBuffer();
		int32 bytesToSend = packet->GetSize();
		int32 bytesSentTotal = 0;
		int32 bytesSent = 0;

		while (bytesSent != bytesToSend)
		{
			if (!m_pSocket->Send(buffer + bytesSentTotal, bytesToSend - bytesSentTotal, bytesSent))
				return false;
			bytesSentTotal += bytesSent;
		}

		ResetTransmitTimer();

		return true;
	}

	/*******************************************
	*					PRIVATE				   *
	********************************************/

	bool ClientTCP2::Receive(char* buffer, int bytesToRead)
	{
		uint32 bytesReceivedTotal = 0;
		int32 bytesReceived = 0;
		while (bytesReceivedTotal != bytesToRead)
		{
			if (m_pSocket->Receive(buffer, bytesToRead - bytesReceivedTotal, bytesReceived))
			{
				bytesReceivedTotal += bytesReceived;
				if (bytesReceivedTotal == 0)
					return false;
			}
			else
			{
				return false;
			}
		}
		RegisterBytesReceived(bytesReceivedTotal);
		return true;
	}

	bool ClientTCP2::ReceivePacket(NetworkPacket* packet)
	{
		packet->Reset();
		char* buffer = packet->GetBuffer();
		if (Receive(buffer, sizeof(PACKET_SIZE)))
		{
			packet->UnPack();
			if (Receive(buffer + sizeof(PACKET_SIZE), packet->GetSize() - sizeof(PACKET_SIZE)))
			{
				return true;
			}
		}
		return false;
	}

	void ClientTCP2::HandlePacket(NetworkPacket* packet)
	{
		PACKET_TYPE type = packet->ReadPacketType();
		if (type == PACKET_TYPE_PING)
		{
			packet->ReadUInt32(m_NrOfPingReceived);
		}

		{
			//for (IClientTCPHandler* handler : m_Handlers)
				m_pHandler->OnClientPacketReceived(this, packet);
		}
	}

	void ClientTCP2::ResetReceiveTimer()
	{
		m_TimerReceived = TCP_THRESHOLD_NANO_SEC;
	}

	void ClientTCP2::ResetTransmitTimer()
	{
		m_TimerTransmit = TCP_PING_INTERVAL_NANO_SEC;
	}

	void ClientTCP2::Tick(Timestamp dt)
	{
		if (ThreadsAreRunning())
		{
			uint64 delta = dt.AsNanoSeconds();
			m_TimerReceived -= delta;
			m_TimerTransmit -= delta;

			if (m_TimerReceived <= 0)
			{
				LOG_WARNING("[ClientTCP]: The time since last packet received is more than 5 Seconds!");
				Disconnect();
			}
			else if (m_TimerTransmit <= 0)
			{
				m_NrOfPingTransmitted++;
				s_PacketPing.Reset();
				s_PacketPing.WriteUInt32(m_NrOfPingTransmitted);
				SendPacket(&s_PacketPing);
				ResetTransmitTimer();
				LOG_MESSAGE("[ClientTCP]: Ping(T%d | R%d)", m_NrOfPingTransmitted, m_NrOfPingReceived);
			}
		}
	}

	ISocketTCP* ClientTCP2::CreateSocket(const std::string& address, uint16 port)
	{
		ISocketTCP* socket = PlatformSocketFactory::CreateSocketTCP();
		if (!socket)
		{
			return nullptr;
		}

		if (!socket->Connect(address, port))
		{
			socket->Close();
			delete socket;
			return nullptr;
		}

		return socket;
	}

	void ClientTCP2::InitStatic()
	{
		s_PacketPing = NetworkPacket(PACKET_TYPE_PING, false);
		s_Clients = new std::set<ClientTCP2*>();
		s_LockClients = new SpinLock();
	}

	///Improve with double buffering to skip lock
	void ClientTCP2::TickStatic(Timestamp dt)
	{
		if (s_LockClients && !s_Clients->empty())
		{
			std::scoped_lock<SpinLock> lock(*s_LockClients);
			for (ClientTCP2* client : *s_Clients)
			{
				client->Tick(dt);
			}
		}
	}

	void ClientTCP2::ReleaseStatic()
	{
		delete s_Clients;
		delete s_LockClients;
	}
}