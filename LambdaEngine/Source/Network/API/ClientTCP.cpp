#include "Network/API/ClientTCP.h"
#include "Network/API/ISocketTCP.h"
#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/IClientTCPHandler.h"
#include "Network/API/NetworkPacket.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientTCP::ClientTCP(IClientTCPHandler* handler) : ClientTCP(handler, nullptr)
	{

	}

	ClientTCP::ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket) :
		m_pHandler(handler),
		m_pSocket(socket),
		m_Stop(false),
		m_pThreadSend(nullptr)
	{
		m_ServerSide = socket != nullptr;

		if(m_ServerSide)
			m_pThread = Thread::Create(std::bind(&ClientTCP::Run, this, "", 0), std::bind(&ClientTCP::OnStopped, this));
	}

	bool ClientTCP::Connect(const std::string& address, uint16 port)
	{
		if (m_pSocket)
		{
			LOG_ERROR_CRIT("Tried to connect an already connected Client!");
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

	bool ClientTCP::IsServerSide() const
	{
		return m_ServerSide;
	}

	void ClientTCP::Run(std::string address, uint16 port)
	{
		if (!IsServerSide())
		{
			m_pSocket = CreateClientSocket(address, port);
			if (!m_pSocket)
			{
				LOG_ERROR_CRIT("Failed to connect to Server!");
				m_pHandler->OnClientFailedConnecting(this);
				return;
			}
		}

		m_pThreadSend = Thread::Create(std::bind(&ClientTCP::RunSend, this), std::bind(&ClientTCP::OnStoppedSend, this));
		m_pHandler->OnClientConnected(this);

		NetworkPacket packet(EPacketType::PACKET_TYPE_UNDEFINED);
		char* buffer = packet.GetBuffer();
		uint16 sizeOffset = sizeof(int16);
		while (!m_Stop)
		{
			packet.ResetHead();
			if (Receive(buffer, sizeOffset))
			{
				packet.UnPack();
				if (Receive(buffer + sizeOffset, packet.GetSize() - sizeOffset))
					m_pHandler->OnClientPacketReceived(this, &packet);
				else
					Disconnect();
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
		return true;
	}

	void ClientTCP::OnStopped()
	{
		m_pThread = nullptr;
		delete m_pSocket;
		m_pSocket = nullptr;
		m_pHandler->OnClientDisconnected(this);
	}

	void ClientTCP::RunSend()
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

			while (!packets.empty())
			{
				NetworkPacket* packet = packets.front();
				packets.pop();

				packet->Pack();
				if (!Send(packet->GetBuffer(), packet->GetSize()))
				{
					Disconnect();
					break;
				}	
			}
		}
	}

	bool ClientTCP::Send(char* buffer, int bytesToSend)
	{
		uint32 bytesSentTotal = 0;
		int32 bytesSent = 0;
		while (bytesSent != bytesToSend)
		{
			if (!m_pSocket->Send(buffer + bytesSentTotal, bytesToSend - bytesSentTotal, bytesSent))
				return false;
			bytesSentTotal += bytesSent;
		}
	}

	void ClientTCP::OnStoppedSend()
	{
		m_pThreadSend = nullptr;
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