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
	ClientRemoteTCP::ClientRemoteTCP(const ClientRemoteDesc& desc, ISocketTCP* pSocket) :
		ClientRemoteBase(desc),
		m_pSocket(pSocket),
		m_pThreadReceiver(nullptr),
		m_PacketManager(desc),
		m_ThreadTerminated(false)
	{
		m_PacketManager.SetEndPoint(pSocket->GetEndPoint());
		m_Transceiver.SetSocket(pSocket);

		pSocket->EnableNaglesAlgorithm(false);

		m_DisconnectedByRemote = true;

		m_pThreadReceiver = Thread::Create(std::bind(&ClientRemoteTCP::RunReceiver, this), nullptr);
	}

	ClientRemoteTCP::~ClientRemoteTCP()
	{
		delete m_pSocket;
		m_pSocket = nullptr;
	}

	void ClientRemoteTCP::RunReceiver()
	{
		IPEndPoint dummy;

		while (!m_pSocket->IsClosed())
		{
			if (!m_Transceiver.ReceiveBegin(dummy))
			{
				Disconnect("Connection Lost");
				break;
			}

			DecodeReceivedPackets();
		}

		//LOG_INFO("[ClientTCPRemote]: Releasing Thread");
		m_ThreadTerminated = true;
		DeleteThis();
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

	bool ClientRemoteTCP::CanDeleteNow()
	{
		return ClientRemoteBase::CanDeleteNow() && m_ThreadTerminated;
	}

	bool ClientRemoteTCP::OnTerminationRequested()
	{
		if (m_pSocket)
			m_pSocket->Close();

		return ClientRemoteBase::OnTerminationRequested();
	}
}