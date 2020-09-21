#include "Networking/API/UDP/ServerNetworkDiscovery.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/INetworkDiscoveryServer.h"

namespace LambdaEngine
{
	ServerNetworkDiscovery::ServerNetworkDiscovery() :
		m_pSocket(nullptr),
		m_SegmentPool(8),
		m_PortOfGameServer(0),
		m_pHandler(nullptr)
	{

	}

	ServerNetworkDiscovery::~ServerNetworkDiscovery()
	{

	}

	bool ServerNetworkDiscovery::Start(const IPEndPoint& endPoint, uint16 portOfGameServer, INetworkDiscoveryServer* pHandler)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			m_IPEndPoint = endPoint;
			m_PortOfGameServer = portOfGameServer;
			m_pHandler = pHandler;
			if (StartThreads())
			{
				LOG_WARNING("[ServerNetworkDiscovery]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerNetworkDiscovery::Stop(const std::string& reason)
	{
		TerminateThreads(reason);

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerNetworkDiscovery::Release()
	{
		TerminateAndRelease("Release Requested");
	}

	bool ServerNetworkDiscovery::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	bool ServerNetworkDiscovery::OnThreadsStarted(std::string& reason)
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				m_Transciver.SetSocket(m_pSocket);
				LOG_INFO("[ServerNetworkDiscovery]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
			reason = "Bind Socket Failed " + m_IPEndPoint.ToString();
			delete m_pSocket;
			m_pSocket = nullptr;
			return false;
		}
		reason = "Create Socket Failed";
		return false;
	}

	void ServerNetworkDiscovery::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
		{
			m_pSocket->Close();
			delete m_pSocket;
			m_pSocket = nullptr;
		}
		LOG_INFO("[ServerNetworkDiscovery]: Stopped");
	}

	void ServerNetworkDiscovery::OnTerminationRequested(const std::string& reason)
	{
		LOG_WARNING("[ServerNetworkDiscovery]: Stopping... [%s]", reason.c_str());
	}

	void ServerNetworkDiscovery::OnReleaseRequested(const std::string& reason)
	{
		Stop(reason);
	}

	void ServerNetworkDiscovery::RunTransmitter()
	{
		
	}

	void ServerNetworkDiscovery::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;
			
			TArray<NetworkSegment*> packets;
			TArray<uint32> acks;

			if (m_Transciver.ReceiveEnd(&m_SegmentPool, packets, acks, &m_Statistics) && packets.GetSize() == 1)
				HandleReceivedPacket(packets[0]);

			m_SegmentPool.FreeSegments(packets);
		}
	}

	void ServerNetworkDiscovery::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_NETWORK_DISCOVERY)
		{
			TQueue<NetworkSegment*> packets;
			TSet<uint32> reliableUIDs;
			NetworkSegment* pResponse = m_SegmentPool.RequestFreeSegment();
			packets.push(pResponse);
			BinaryEncoder encoder(pResponse);

			encoder.WriteUInt16(m_PortOfGameServer);
			m_pHandler->OnNetworkDiscoveryPreTransmit(encoder);

			m_Transciver.Transmit(&m_SegmentPool, packets, reliableUIDs, m_IPEndPoint, &m_Statistics);
		}
	}
}