#include "Networking/API/UDP/ClientNetworkDiscovery.h"
#include "Networking/API/UDP/INetworkDiscoveryClient.h"
#include "Networking/API/PlatformNetworkUtils.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	ClientNetworkDiscovery::ClientNetworkDiscovery() : 
		m_SegmentPool(8),
		m_pSocket(nullptr),
		m_IPEndPoint(),
		m_Statistics(),
		m_Transceiver(),
		m_NameOfGame(),
		m_Lock(),
		m_pHandler(),
		m_TimeOfLastSearch(),
		m_SearchInterval()
	{

	}

	ClientNetworkDiscovery::~ClientNetworkDiscovery()
	{
		LOG_INFO("[ClientNetworkDiscovery]: Released");
	}

	bool ClientNetworkDiscovery::Connect(const IPEndPoint& endPoint, const String& nameOfGame, INetworkDiscoveryClient* pHandler, Timestamp searchInterval)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			m_IPEndPoint = endPoint;
			m_NameOfGame = nameOfGame;
			m_pHandler = pHandler;
			m_SearchInterval = searchInterval;
			if (StartThreads())
			{
				LOG_WARNING("[ClientNetworkDiscovery]: Connecting...");
				return true;
			}
		}
		return false;
	}

	void ClientNetworkDiscovery::Release()
	{
		NetWorker::TerminateAndRelease("Release Requested");
	}

	bool ClientNetworkDiscovery::OnThreadsStarted(std::string& reason)
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			IPEndPoint endPoint(IPAddress::ANY, 0);
			if (m_pSocket->Bind(endPoint))
			{
				if (m_pSocket->EnableBroadcast(true))
				{
					m_Transceiver.SetSocket(m_pSocket);
					return true;
				}
				reason = "Enable Broadcast Failed";
				delete m_pSocket;
				m_pSocket = nullptr;
				return false;
			}
			reason = "Bind Socket Failed " + endPoint.ToString();
			delete m_pSocket;
			m_pSocket = nullptr;
			return false;
		}
		reason = "Create Socket Failed";
		return false;
	}

	void ClientNetworkDiscovery::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			TQueue<NetworkSegment*> packets;
			TSet<uint32> reliableUIDs;

#ifdef LAMBDA_DEBUG
			NetworkSegment* pResponse = m_SegmentPool.RequestFreeSegment("ClientNetworkDiscovery");
#else
			NetworkSegment* pResponse = m_SegmentPool.RequestFreeSegment();
#endif

			pResponse->GetHeader().Type = NetworkSegment::TYPE_NETWORK_DISCOVERY;
			packets.push(pResponse);

			BinaryEncoder encoder(pResponse);
			encoder.WriteString(m_NameOfGame);

			m_Transceiver.Transmit(&m_SegmentPool, packets, reliableUIDs, m_IPEndPoint, &m_Statistics);

			YieldTransmitter();
		}
	}

	void ClientNetworkDiscovery::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transceiver.ReceiveBegin(sender))
				continue;

			TArray<NetworkSegment*> packets;
			TArray<uint32> acks;

			if (m_Transceiver.ReceiveEnd(&m_SegmentPool, packets, acks, &m_Statistics) && packets.GetSize() == 1)
				HandleReceivedPacket(sender, packets[0]);

			m_SegmentPool.FreeSegments(packets);
		}
	}

	void ClientNetworkDiscovery::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (m_pSocket)
		{
			m_pSocket->Close();
			delete m_pSocket;
			m_pSocket = nullptr;
		}

		LOG_INFO("[ClientNetworkDiscovery]: Disconnected");
	}

	void ClientNetworkDiscovery::OnTerminationRequested(const std::string& reason)
	{
		LOG_WARNING("[ClientNetworkDiscovery]: Disconnecting... [%s]", reason.c_str());
	}

	void ClientNetworkDiscovery::OnReleaseRequested(const std::string& reason)
	{
		UNREFERENCED_VARIABLE(reason);

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ClientNetworkDiscovery::HandleReceivedPacket(const IPEndPoint& sender, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_NETWORK_DISCOVERY)
		{
			BinaryDecoder decoder(pPacket);
			if (decoder.ReadString() == m_NameOfGame)
			{
				m_pHandler->OnServerFound(decoder, IPEndPoint(sender.GetAddress(), decoder.ReadUInt16()));
			}
		}
	}

	void ClientNetworkDiscovery::Tick(Timestamp delta)
	{
		UNREFERENCED_VARIABLE(delta);

		if (EngineLoop::GetTimeSinceStart() - m_TimeOfLastSearch > m_SearchInterval)
		{
			m_TimeOfLastSearch = EngineLoop::GetTimeSinceStart();
			Flush();
		}
	}
}