#include "Network/API/ClientUDP.h"
#include "Network/API/PlatformSocketFactory.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(const std::string& address, uint16 port, IClientUDPHandler* handler) :
		m_pHandler(handler),
		m_pSocket(nullptr),
		m_Stop(false),
		m_Release(false),
		m_pThread(nullptr),
		m_pThreadSend(nullptr),
		m_NrOfPingReceived(0),
		m_NrOfPingTransmitted(0),
		m_NrOfPacketsTransmitted(0),
		m_NrOfPacketsReceived(0),
		m_NrOfBytesTransmitted(0),
		m_NrOfBytesReceived(0),
		m_Address(address),
		m_Port(port)
	{
		m_pSocket = CreateClientSocket();
		if (!m_pSocket)
		{
			LOG_ERROR("[ClientUDP]: Failed to connect to Server!");
			return;
		}
		m_pThread = Thread::Create(std::bind(&ClientUDP::Run, this, address, port), std::bind(&ClientUDP::OnStopped, this));
	}

	ClientUDP::~ClientUDP()
	{
		if (!m_Release)
			LOG_ERROR("[ClientUDP]: Do not use delete on a ClientUDP. Use the Release() function!");
		LOG_MESSAGE("~ClientUDP()");
	}

	void ClientUDP::SendPacket(NetworkPacket* packet)
	{
		if (!m_Stop)
		{
			std::scoped_lock<SpinLock> lock(m_LockPacketsToSend);
			m_PacketsToSend.push(packet);
			if (m_pThreadSend)
				m_pThreadSend->Notify();
		}
	}

	bool ClientUDP::SendPacketImmediately(NetworkPacket* packet)
	{
		packet->Pack();
		bool result = Transmit(packet->GetBuffer(), packet->GetSize());
		DeletePacket(packet);
		return result;
	}

	bool ClientUDP::IsServerSide() const
	{
		return m_ServerSide;
	}

	void ClientUDP::Release()
	{
		m_Release = true;
		m_Stop = true;
		if (m_pThreadSend == nullptr && m_pThread == nullptr)
		{
			delete this;
		}
	}

	int32 ClientUDP::GetBytesSent() const
	{
		return m_NrOfBytesTransmitted;
	}

	int32 ClientUDP::GetBytesReceived() const
	{
		return m_NrOfBytesReceived;
	}

	int32 ClientUDP::GetPacketsSent() const
	{
		return m_NrOfPacketsTransmitted;
	}

	int32 ClientUDP::GetPacketsReceived() const
	{
		return m_NrOfPacketsReceived;
	}

	void ClientUDP::Run(std::string address, uint16 port)
	{
		m_pThreadSend = Thread::Create(std::bind(&ClientUDP::RunTransmit, this), std::bind(&ClientUDP::OnStoppedSend, this));

		NetworkPacket packet(PACKET_TYPE_UNDEFINED, false);
		SendPacketImmediately(&packet);
		int32 bytesReceived = 0;
		while (!m_Stop)
		{
			if (m_pSocket->ReceiveFrom(packet.GetBuffer(), MAXIMUM_PACKET_SIZE, bytesReceived, address, port))
			{
				packet.Reset();
				packet.UnPack();
				if (bytesReceived != packet.GetSize())
				{
					m_pHandler->OnClientPacketReceivedUDP(this, &packet);
				}
			}
			else
			{
				m_Stop = true;
			}
		}
	}

	void ClientUDP::OnStopped()
	{
		m_pThread = nullptr;
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_WARNING("[ClientUDP]: Disconnected!");

		if (m_pThreadSend)
			m_pThreadSend->Notify();

		if (m_Release)
			Release();
	}

	void ClientUDP::RunTransmit()
	{
		while (!m_pThreadSend) {}

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

	void ClientUDP::OnStoppedSend()
	{
		m_pThreadSend = nullptr;
		if (m_Release)
			Release();
	}

	bool ClientUDP::Transmit(char* buffer, int bytesToSend)
	{
		uint32 bytesSentTotal = 0;
		int32 bytesSent = 0;
		while (bytesSent != bytesToSend)
		{
			if (!m_pSocket->SendTo(buffer + bytesSentTotal, bytesToSend - bytesSentTotal, bytesSent, m_Address, m_Port))
				return false;
			bytesSentTotal += bytesSent;
		}
		m_NrOfPacketsTransmitted++;
		m_NrOfBytesTransmitted += bytesSentTotal;
		LOG_WARNING("[ClientUDP]: Transmit! %d", m_NrOfPacketsTransmitted);
		return true;
	}

	void ClientUDP::TransmitPackets(std::queue<NetworkPacket*>* packets)
	{
		while (!packets->empty())
		{
			NetworkPacket* packet = packets->front();
			packets->pop();

			if (!SendPacketImmediately(packet))
			{
				m_Stop = true;
				DeletePackets(packets);
				break;
			}
		}
	}

	void ClientUDP::DeletePackets(std::queue<NetworkPacket*>* packets)
	{
		while (!packets->empty())
		{
			NetworkPacket* packet = packets->front();
			packets->pop();
			DeletePacket(packet);
		}
	}

	void ClientUDP::DeletePacket(NetworkPacket* packet)
	{
		if (packet->ShouldAutoDelete())
			delete packet;
	}

	ISocketUDP* ClientUDP::CreateClientSocket()
	{
		return PlatformSocketFactory::CreateSocketUDP();
	}
}
