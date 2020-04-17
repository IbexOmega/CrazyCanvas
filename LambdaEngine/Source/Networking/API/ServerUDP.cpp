#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IServerUDPHandler.h"

#include "Log/Log.h"

#include "Networking/API/BinaryEncoder.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(IServerUDPHandler* pHandler, uint16 packetPerClient) :
		m_pHandler(pHandler),
		m_PacketsPerClient(packetPerClient),
		m_pSocket(nullptr),
		m_Accepting(true)
	{

	}

	ServerUDP::~ServerUDP()
	{

	}

	bool ServerUDP::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerUDP]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerUDP::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	bool ServerUDP::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerUDP::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerUDP::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	bool ServerUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				LOG_INFO("[ServerUDP]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
		}
		return false;
	}

	void ServerUDP::RunReceiver()
	{
		int32 bytesReceived = 0;
		int32 packetsReceived = 0;
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_pSocket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, bytesReceived, sender))
			{
				TerminateThreads();
				break;
			}

			ClientUDPRemote* pClient = nullptr;
			auto pIterator = m_Clients.find(sender);
			if (pIterator != m_Clients.end())
			{
				pClient = pIterator->second;
			}
			else
			{
				IClientUDPHandler* pHandler = m_pHandler->CreateClientUDPHandler();
				pClient = DBG_NEW ClientUDPRemote(m_PacketsPerClient, sender, pHandler, this);
				m_Clients.insert({ sender, pClient });
			}

			pClient->OnDataReceived(m_pReceiveBuffer, bytesReceived);
		}
	}

	void ServerUDP::RunTranmitter()
	{
		int32 bytesWritten = 0;
		int32 bytesSent = 0;
		bool done = false;

		while (!ShouldTerminate())
		{
			for (auto& tuple : m_Clients)
			{
				tuple.second->SendPackets(m_pSendBuffer);
			}
			YieldTransmitter();
		}
	}
	
	void ServerUDP::OnThreadsTurminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerUDP]: Stopped");
	}

	void ServerUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ServerUDP]: Stopping...");
	}

	void ServerUDP::OnReleaseRequested()
	{
		Stop();
	}

	void ServerUDP::Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite)
	{
		int32 bytesSent = 0;
		if (!m_pSocket->SendTo(data, bytesToWrite, bytesSent, ipEndPoint))
		{
			TerminateThreads();
		}
		else if (bytesToWrite != bytesSent)
		{
			TerminateThreads();
		}
	}

	ServerUDP* ServerUDP::Create(IServerUDPHandler* pHandler, uint16 packetsPerClient)
	{
		return DBG_NEW ServerUDP(pHandler, packetsPerClient);
	}
}