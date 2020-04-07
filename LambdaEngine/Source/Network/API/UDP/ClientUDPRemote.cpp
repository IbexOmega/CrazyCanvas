#include "Network/API/UDP/ClientUDPRemote.h"
#include "Network/API/UDP/IClientUDPHandler.h"
#include "Network/API/UDP/ServerUDP.h"

namespace LambdaEngine
{
	ClientUDPRemote::ClientUDPRemote(const std::string& address, uint16 port, uint64 hash, ServerUDP* server, IClientUDPHandler* handler) :
		m_Address(address),
		m_Port(port),
		m_Hash(hash),
		m_pServer(server),
		m_pHandler(handler),
		m_NrOfBytesTransmitted(0),
		m_NrOfBytesReceived(0),
		m_NrOfPacketsTransmitted(0),
		m_NrOfPacketsReceived(0)
	{

	}

	ClientUDPRemote::~ClientUDPRemote()
	{
		LOG_WARNING("~ClientUDPRemote()");
	}

	bool ClientUDPRemote::Start(const std::string& address, uint16 port)
	{
		return false;
	}

	bool ClientUDPRemote::SendPacket(NetworkPacket* packet)
	{
		packet->SetDestination(GetAddress(), GetPort());
		return m_pServer->SendPacket(packet);
	}

	bool ClientUDPRemote::SendPacketImmediately(NetworkPacket* packet)
	{
		packet->SetDestination(GetAddress(), GetPort());
		return m_pServer->SendPacketImmediately(packet);
	}

	void ClientUDPRemote::Release()
	{
		m_pServer->OnClientReleased(this);
		ReleaseInternal();
	}

	bool ClientUDPRemote::IsServerSide() const
	{
		return true;
	}

	const std::string& ClientUDPRemote::GetAddress() const
	{
		return m_Address;
	}

	uint16 ClientUDPRemote::GetPort() const
	{
		return m_Port;
	}

	int32 ClientUDPRemote::GetBytesSent() const
	{
		return m_NrOfBytesTransmitted;
	}

	int32 ClientUDPRemote::GetBytesReceived() const
	{
		return m_NrOfBytesReceived;
	}

	int32 ClientUDPRemote::GetPacketsSent() const
	{
		return m_NrOfPacketsTransmitted;
	}

	int32 ClientUDPRemote::GetPacketsReceived() const
	{
		return m_NrOfPacketsReceived;
	}

	uint64 ClientUDPRemote::GetHash() const
	{
		return m_Hash;
	}

	void ClientUDPRemote::ReleaseInternal()
	{
		delete this;
	}

	void ClientUDPRemote::OnPacketReceived(NetworkPacket* packet)
	{
		m_pHandler->OnClientPacketReceivedUDP(this, packet);
	}
}
