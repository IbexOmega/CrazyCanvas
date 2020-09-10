#include "Networking/API/TCP/ClientRemoteTCP.h"
#include "Networking/API/TCP/ServerTCP.h"
#include "Networking/API/TCP/ISocketTCP.h"

#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkChallenge.h"

#include "Threading/API/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientRemoteTCP::ClientRemoteTCP(uint16 packetPoolSize, ISocketTCP* pSocket, ServerTCP* pServer) :
		ClientRemoteBase(pServer),
		m_pSocket(pSocket),
		m_pThreadReceiver(nullptr),
		m_PacketManager({ packetPoolSize, 0 })
	{
		m_PacketManager.SetEndPoint(pSocket->GetEndPoint());
		m_Transceiver.SetSocket(pSocket);

		pSocket->EnableNaglesAlgorithm(false);

		m_pThreadReceiver = Thread::Create(
			std::bind(&ClientRemoteTCP::RunReceiver, this),
			std::bind(&ClientRemoteTCP::OnThreadReceiverTerminated, this)
		);
	}

	void ClientRemoteTCP::RunReceiver()
	{
		IPEndPoint dummy;

		while (m_State != STATE_DISCONNECTED)
		{
			if (!m_Transceiver.ReceiveBegin(dummy))
				continue;

			DecodeReceivedPackets();
		}

		LOG_INFO("[ClientTCPRemote]: Thread Ended");
	}

	void ClientRemoteTCP::OnThreadReceiverTerminated()
	{
		m_pThreadReceiver = nullptr;
	}

	PacketManagerBase* ClientRemoteTCP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	const PacketManagerBase* ClientRemoteTCP::GetPacketManager() const
	{
		return &m_PacketManager;
	}

	PacketTransceiverBase* ClientRemoteTCP::GetTransceiver()
	{
		return &m_Transceiver;
	}
}