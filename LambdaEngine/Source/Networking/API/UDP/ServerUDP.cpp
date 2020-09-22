#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/UDP/ClientRemoteUDP.h"
#include "Networking/API/UDP/ISocketUDP.h"
#include "Networking/API/UDP/ServerUDP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(const ServerDesc& desc) :
		ServerBase(desc),
		m_Transciver()
	{
		
	}

	ServerUDP::~ServerUDP()
	{
		
	}

	void ServerUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ServerUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	ISocket* ServerUDP::SetupSocket(std::string& reason)
	{
		ISocketUDP* pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (pSocket)
		{
			if (pSocket->Bind(GetEndPoint()))
			{
				m_Transciver.SetSocket(pSocket);
				LOG_INFO("[ServerUDP]: Started %s", GetEndPoint().ToString().c_str());
				return pSocket;
			}
			reason = "Bind Socket Failed " + GetEndPoint().ToString();
			delete pSocket;
			return nullptr;
		}
		reason = "Create Socket Failed";
		return nullptr;
	}

	void ServerUDP::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			bool newConnection = false;
			ClientRemoteUDP* pClient = GetOrCreateClient(sender, newConnection);

			if (newConnection)
			{
				HandleNewConnection(pClient);
			}

			pClient->DecodeReceivedPackets();
		}
	}

	ClientRemoteUDP* ServerUDP::GetOrCreateClient(const IPEndPoint& sender, bool& newConnection)
	{
		ClientRemoteBase* pClient = GetClient(sender);
		if (pClient)
		{
			newConnection = false;
			return (ClientRemoteUDP*)pClient;
		}
		else
		{
			newConnection = true;

			ClientRemoteDesc desc = {};
			memcpy(&desc, &GetDescription(), sizeof(ServerDesc));
			desc.Server = this;

			return DBG_NEW ClientRemoteUDP(desc, sender, &m_Transciver);
		}
	}
}