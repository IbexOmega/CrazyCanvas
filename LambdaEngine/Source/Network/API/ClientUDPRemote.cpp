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
		m_pHandler(handler)
	{

	}

	ClientUDPRemote::~ClientUDPRemote()
	{

	}

	bool ClientUDPRemote::Start(const std::string& address, uint16 port)
	{
		return false;
	}

	bool ClientUDPRemote::SendPacket(NetworkPacket* packet)
	{
		return false;
	}

	bool ClientUDPRemote::SendPacketImmediately(NetworkPacket* packet)
	{
		return false;
	}

	void ClientUDPRemote::Release()
	{
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
		return int32();
	}

	int32 ClientUDPRemote::GetBytesReceived() const
	{
		return int32();
	}

	int32 ClientUDPRemote::GetPacketsSent() const
	{
		return int32();
	}

	int32 ClientUDPRemote::GetPacketsReceived() const
	{
		return int32();
	}

	uint64 ClientUDPRemote::GetHash() const
	{
		return m_Hash;
	}

	void ClientUDPRemote::OnPacketReceived(NetworkPacket* packet)
	{
		m_pHandler->OnClientPacketReceivedUDP(this, packet);
	}
}
