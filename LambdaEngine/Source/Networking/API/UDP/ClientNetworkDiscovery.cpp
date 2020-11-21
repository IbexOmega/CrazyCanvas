#include "Networking/API/UDP/ClientNetworkDiscovery.h"
#include "Networking/API/UDP/INetworkDiscoveryClient.h"
#include "Networking/API/PlatformNetworkUtils.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	ClientNetworkDiscovery::ClientNetworkDiscovery() : 
		m_SegmentPool(8),
		m_pSocket(nullptr),
		m_pEndPoints(nullptr),
		m_Statistics(),
		m_Transceiver(),
		m_NameOfGame(),
		m_Lock(),
		m_pHandler(),
		m_TimeOfLastSearch(),
		m_SearchInterval(),
		m_BufferIndex(0),
		m_ReceivedPackets(),
		m_LockReceivedPackets()
	{
		m_Transceiver.SetIgnoreSaltMissmatch(true);
	}

	ClientNetworkDiscovery::~ClientNetworkDiscovery()
	{
		LOG_INFO("[ClientNetworkDiscovery]: Released");
	}

	bool ClientNetworkDiscovery::Connect(const TSet<IPEndPoint>* pEndPoints, SpinLock* pLock, const String& nameOfGame, INetworkDiscoveryClient* pHandler, Timestamp searchInterval)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			m_pEndPoints = pEndPoints;
			m_pLockEndPoints = pLock;
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
			{
				std::scoped_lock<SpinLock> lock(*m_pLockEndPoints);
				for (const IPEndPoint& endpoint : *m_pEndPoints)
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
					encoder.WriteBool(*endpoint.GetAddress() == *IPAddress::BROADCAST);

					m_Transceiver.Transmit(&m_SegmentPool, packets, reliableUIDs, endpoint, &m_Statistics);
				}
			}
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
			TSet<uint32> acks;

			if (m_Transceiver.ReceiveEnd(&m_SegmentPool, packets, acks, &m_Statistics) && packets.GetSize() == 1)
			{
				if (!HandleReceivedPacket(sender, packets[0]))
				{
#ifdef LAMBDA_CONFIG_DEBUG
					m_SegmentPool.FreeSegment(packets[0], "ClientNetworkDiscovery::RunReceiver");
#else
					m_SegmentPool.FreeSegment(packets[0]);
#endif		
				}
			}
			else
			{
#ifdef LAMBDA_CONFIG_DEBUG
				m_SegmentPool.FreeSegments(packets, "ClientNetworkDiscovery::RunReceiver2");
#else
				m_SegmentPool.FreeSegments(packets);
#endif				
			}
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

	bool ClientNetworkDiscovery::HandleReceivedPacket(const IPEndPoint& sender, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_NETWORK_DISCOVERY)
		{
			BinaryDecoder decoder(pPacket);
			if (decoder.ReadString() == m_NameOfGame)
			{
				Timestamp ping = EngineLoop::GetTimeSinceStart() - m_TimeOfLastSearch;
				std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
				m_ReceivedPackets[m_BufferIndex].PushBack({ decoder, sender, ping });
				return true;
			}
		}
		return false;
	}

	void ClientNetworkDiscovery::FixedTick(Timestamp delta)
	{
		UNREFERENCED_VARIABLE(delta);

		if (EngineLoop::GetTimeSinceStart() - m_TimeOfLastSearch > m_SearchInterval)
		{
			m_TimeOfLastSearch = EngineLoop::GetTimeSinceStart();
			Flush();
		}

		HandleReceivedPacketsMainThread();
	}

	void ClientNetworkDiscovery::HandleReceivedPacketsMainThread()
	{
		TArray<Packet>& packets = m_ReceivedPackets[m_BufferIndex];

		{
			std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
			m_BufferIndex = (m_BufferIndex + 1) % 2;
		}

		for (Packet& packet : packets)
		{
			BinaryDecoder& decoder = packet.Decoder;
			bool isLAN = decoder.ReadBool();
			IPEndPoint endpoint(packet.Sender.GetAddress(), decoder.ReadUInt16());
			uint64 serverUID = decoder.ReadUInt64();
			m_pHandler->OnServerFound(decoder, endpoint, serverUID, packet.Ping, isLAN);
#ifdef LAMBDA_CONFIG_DEBUG
			m_SegmentPool.FreeSegment(decoder.GetPacket(), "ClientNetworkDiscovery::HandleReceivedPacketsMainThread");
#else
			m_SegmentPool.FreeSegment(decoder.GetPacket());
#endif	
		}
		packets.Clear();
	}
}