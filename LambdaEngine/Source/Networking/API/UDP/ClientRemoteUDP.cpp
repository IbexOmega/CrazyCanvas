#include "Networking/API/UDP/ClientRemoteUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"
#include "Networking/API/UDP/ServerUDP.h"
#include "Networking/API/UDP/ISocketUDP.h"

#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkChallenge.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientRemoteUDP::ClientRemoteUDP(const ClientRemoteDesc& desc, const IPEndPoint& ipEndPoint, PacketTransceiverUDP* pTransceiver) :
		ClientRemoteBase(desc),
		m_pTransceiver(pTransceiver),
		m_PacketManager(desc)
	{
		m_PacketManager.SetEndPoint(ipEndPoint);
	}

	PacketManagerBase* ClientRemoteUDP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	const PacketManagerBase* ClientRemoteUDP::GetPacketManager() const
	{
		return &m_PacketManager;
	}

	PacketTransceiverBase* ClientRemoteUDP::GetTransceiver()
	{
		return m_pTransceiver;
	}
}