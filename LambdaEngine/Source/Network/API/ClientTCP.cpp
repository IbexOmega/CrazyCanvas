#include "Network/API/ClientTCP.h"
#include "Network/API/ISocketTCP.h"
#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/IClientTCPHandler.h"
#include "Network/API/NetworkPacket.h"

#include "Time/API/Clock.h"
#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	NetworkPacket ClientTCP::s_PacketPing(PACKET_TYPE_PING, false);
	std::set<ClientTCP*>* ClientTCP::s_Clients	= nullptr;
	SpinLock* ClientTCP::s_LockClients			= nullptr;

	ClientTCP::ClientTCP(IClientTCPHandler* handler) : ClientTCP({ handler }, nullptr)
	{

	}

	ClientTCP::ClientTCP(const std::set<IClientTCPHandler*>& handlers, ISocketTCP* socket) :
		m_Handlers(handlers),
		m_pSocket(socket),
		m_Stop(false),
		m_Release(false),
		m_pThread(nullptr),
		m_pThreadSend(nullptr),
		m_TimerReceived(0),
		m_TimerTransmit(0),
		m_NrOfPingReceived(0),
		m_NrOfPingTransmitted(0),
		m_NrOfPacketsTransmitted(0),
		m_NrOfPacketsReceived(0),
		m_NrOfBytesTransmitted(0),
		m_NrOfBytesReceived(0)
	{
		{
			std::scoped_lock<SpinLock> lock(*s_LockClients);
			s_Clients->insert(this);
		}

		m_ServerSide = socket != nullptr;

		if(m_ServerSide)
			m_pThread = Thread::Create(std::bind(&ClientTCP::Run, this, "", 0), std::bind(&ClientTCP::OnStopped, this));
	}

	ClientTCP::~ClientTCP()
	{
		if(!m_Release)
			LOG_ERROR("[ClientTCP]: Do not use delete on a ClientTCP. Use the Release() function!");
		LOG_MESSAGE("~ClientTCP()");

		std::scoped_lock<SpinLock> lock(*s_LockClients);
		s_Clients->erase(this);
	}

	bool ClientTCP::Connect(const std::string& address, uint16 port)
	{
		if (m_pSocket)
		{
			LOG_ERROR("[ClientTCP]: Tried to connect an already connected Client!");
			return false;
		}

		m_pThread = Thread::Create(std::bind(&ClientTCP::Run, this, address, port), std::bind(&ClientTCP::OnStopped, this));
		return true;
	}

	void ClientTCP::Disconnect()
	{
		if (!m_Stop)
		{
			m_Stop = true;
			m_pSocket->Close();
			if (m_pThreadSend)
				m_pThreadSend->Notify();
		}
	}

	bool ClientTCP::IsServerSide() const
	{
		return m_ServerSide;
	}

	void ClientTCP::Release()
	{
		m_Release = true;
		Disconnect();
		if (m_pThreadSend == nullptr && m_pThread == nullptr)
		{
			delete this;
		}
	}

	bool ClientTCP::IsConnected() const
	{
		return !m_Stop && m_pThread && m_pThreadSend;
	}

	int32 ClientTCP::GetBytesSent() const
	{
		return m_NrOfBytesTransmitted;
	}

	int32 ClientTCP::GetBytesReceived() const
	{
		return m_NrOfBytesReceived;
	}

	int32 ClientTCP::GetPacketsSent() const
	{
		return m_NrOfPacketsTransmitted;
	}

	int32 ClientTCP::GetPacketsReceived() const
	{
		return m_NrOfPacketsReceived;
	}

	void ClientTCP::Update(Timestamp dt)
	{
		if (IsConnected())
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

	void ClientTCP::ResetReceiveTimer()
	{
		m_TimerReceived = 5000000000;
	}

	void ClientTCP::ResetTransmitTimer()
	{
		m_TimerTransmit = 1000000000;
	}

	void ClientTCP::Run(std::string address, uint16 port)
	{
		if (!IsServerSide())
		{
			m_pSocket = CreateClientSocket(address, port);
			if (!m_pSocket)
			{
				LOG_ERROR("[ClientTCP]: Failed to connect to Server!");
				for(IClientTCPHandler* handler : m_Handlers)
					handler->OnClientFailedConnecting(this);
				return;
			}
		}
		LOG_INFO("[ClientTCP]: Connected!");
		m_pSocket->DisableNaglesAlgorithm();
		ResetReceiveTimer();
		ResetTransmitTimer();
		m_pThreadSend = Thread::Create(std::bind(&ClientTCP::RunTransmit, this), std::bind(&ClientTCP::OnStoppedSend, this));

		for (IClientTCPHandler* handler : m_Handlers)
			handler->OnClientConnected(this);

		NetworkPacket packet(EPacketType::PACKET_TYPE_UNDEFINED);
		while (!m_Stop)
		{
			if (ReceivePacket(&packet))
			{
				m_NrOfPacketsReceived++;
				ResetReceiveTimer();
				HandlePacket(&packet);
			}
			else
			{
				Disconnect();
			}
		}
	}

	bool ClientTCP::Receive(char* buffer, int bytesToRead)
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
		m_NrOfBytesReceived += bytesReceivedTotal;
		return true;
	}

	bool ClientTCP::ReceivePacket(NetworkPacket* packet)
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

	void ClientTCP::HandlePacket(NetworkPacket* packet)
	{
		PACKET_TYPE type = packet->ReadPacketType();
		if (type == PACKET_TYPE_PING)
		{
			packet->ReadUInt32(m_NrOfPingReceived);
		}

		for (IClientTCPHandler* handler : m_Handlers)
			handler->OnClientPacketReceived(this, packet);
	}

	void ClientTCP::OnStopped()
	{
		m_pThread = nullptr;
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_WARNING("[ClientTCP]: Disconnected!");

		for (IClientTCPHandler* handler : m_Handlers)
			handler->OnClientDisconnected(this);

		if (m_Release)
			Release();
	}

	void ClientTCP::RunTransmit()
	{
		while (!m_pThreadSend){}

		while (!m_Stop)
		{
			if (m_PacketsToSend.empty())
				m_pThreadSend->Wait();

			std::queue<NetworkPacket*> packets;
			{
				std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
				packets = m_PacketsToSend;
				std::queue<NetworkPacket*>().swap(m_PacketsToSend);
			}

			TransmitPackets(&packets);
		}

		DeletePackets(&m_PacketsToSend);
	}

	bool ClientTCP::Transmit(char* buffer, int bytesToSend)
	{
		uint32 bytesSentTotal = 0;
		int32 bytesSent = 0;
		while (bytesSent != bytesToSend)
		{
			if (!m_pSocket->Send(buffer + bytesSentTotal, bytesToSend - bytesSentTotal, bytesSent))
				return false;
			bytesSentTotal += bytesSent;
		}
		ResetTransmitTimer();
		m_NrOfPacketsTransmitted++;
		m_NrOfBytesTransmitted += bytesSentTotal;
        return true;
	}

	void ClientTCP::SendPacket(NetworkPacket* packet)
	{
		if (!m_Stop)
		{
			std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
			m_PacketsToSend.push(packet);
			if (m_pThreadSend)
				m_pThreadSend->Notify();
		}
	}

	bool ClientTCP::SendPacketImmediately(NetworkPacket* packet)
	{
		packet->Pack();
		bool result = Transmit(packet->GetBuffer(), packet->GetSize());
		DeletePacket(packet);
		return result;
	}

	void ClientTCP::TransmitPackets(std::queue<NetworkPacket*>* packets)
	{
		while (!packets->empty())
		{
			NetworkPacket* packet = packets->front();
			packets->pop();

			if (!SendPacketImmediately(packet))
			{
				Disconnect();
				DeletePackets(packets);
				break;
			}
		}
	}

	void ClientTCP::DeletePackets(std::queue<NetworkPacket*>* packets)
	{
		while (!packets->empty())
		{
			NetworkPacket* packet = packets->front();
			packets->pop();
			DeletePacket(packet);
		}
	}

	void ClientTCP::DeletePacket(NetworkPacket* packet)
	{
		if (packet->ShouldAutoDelete())
			delete packet;
	}

	void ClientTCP::OnStoppedSend()
	{
		m_pThreadSend = nullptr;
		if (m_Release)
			Release();
	}

	void ClientTCP::Init()
	{
		s_PacketPing = NetworkPacket(PACKET_TYPE_PING, false);
		s_Clients = new std::set<ClientTCP*>();
		s_LockClients = new SpinLock();
	}

	void ClientTCP::Tick(Timestamp dt)
	{
		if (s_LockClients && !s_Clients->empty())
		{
			std::scoped_lock<SpinLock> lock(*s_LockClients);
			for (ClientTCP* client : *s_Clients)
			{
				client->Update(dt);
			}
		}	
	}

	void ClientTCP::ReleaseStatic()
	{
		delete s_Clients;
		delete s_LockClients;
	}

	ISocketTCP* ClientTCP::CreateClientSocket(const std::string& address, uint16 port)
	{
		ISocketTCP* socket = PlatformSocketFactory::CreateSocketTCP();
		if (!socket)
			return nullptr;

		if (!socket->Connect(address, port))
		{
			socket->Close();
			delete socket;
			return nullptr;
		}

		return socket;
	}
}