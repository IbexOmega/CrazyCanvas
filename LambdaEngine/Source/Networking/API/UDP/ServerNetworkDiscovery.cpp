#include "Networking/API/UDP/ServerNetworkDiscovery.h"
#include "Networking/API/UDP/INetworkDiscoveryServer.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

namespace LambdaEngine
{
	ServerNetworkDiscovery::ServerNetworkDiscovery() :
		m_pSocket(nullptr),
		m_SegmentPool(256),
		m_PortOfGameServer(0),
		m_pHandler(nullptr),
		m_IPEndPoint(),
		m_Transceiver(),
		m_Statistics(),
		m_Lock(),
		m_NameOfGame(),
		m_ServerUID(0)
	{
		m_Transceiver.SetIgnoreSaltMissmatch(true);
		m_ServerUID = Random::UInt64();
	}

	ServerNetworkDiscovery::~ServerNetworkDiscovery()
	{
		LOG_INFO("[ServerNetworkDiscovery]: Released");
	}

	bool ServerNetworkDiscovery::Start(const IPEndPoint& endPoint, const String& nameOfGame, uint16 portOfGameServer, INetworkDiscoveryServer* pHandler)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			m_IPEndPoint = endPoint;
			m_NameOfGame = nameOfGame;
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
				m_Transceiver.SetSocket(m_pSocket);
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
			if (!m_Transceiver.ReceiveBegin(sender))
				continue;
			
			TArray<NetworkSegment*> packets;
			TSet<uint32> acks;

			if (m_Transceiver.ReceiveEnd(&m_SegmentPool, packets, acks, &m_Statistics) && packets.GetSize() == 1)
				HandleReceivedPacket(sender, packets[0]);

#ifdef LAMBDA_CONFIG_DEBUG
			m_SegmentPool.FreeSegments(packets, "ServerNetworkDiscovery::RunReceiver");
#else
			m_SegmentPool.FreeSegments(packets);
#endif
		}
	}

	void ServerNetworkDiscovery::HandleReceivedPacket(const IPEndPoint& sender, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_NETWORK_DISCOVERY)
		{
			BinaryDecoder decoder(pPacket);
			if (decoder.ReadString() == m_NameOfGame)
			{
				std::set<NetworkSegment*, NetworkSegmentUIDOrder> packets;
				TSet<uint32> reliableUIDs;

#ifdef LAMBDA_DEBUG
				NetworkSegment* pResponse = m_SegmentPool.RequestFreeSegment("ClientNetworkDiscovery");
#else
				NetworkSegment* pResponse = m_SegmentPool.RequestFreeSegment();
#endif

				pResponse->GetHeader().Type = NetworkSegment::TYPE_NETWORK_DISCOVERY;
				packets.insert(pResponse);

				BinaryEncoder encoder(pResponse);
				encoder.WriteString(m_NameOfGame);
				encoder.WriteBool(decoder.ReadBool());
				encoder.WriteUInt16(m_PortOfGameServer);
				encoder.WriteUInt64(m_ServerUID);
				m_pHandler->OnNetworkDiscoveryPreTransmit(encoder);

				m_Transceiver.Transmit(&m_SegmentPool, packets, reliableUIDs, sender, &m_Statistics);
			}
		}
	}
}