#include "RemoteClientUDP.h"

#include "IClientUDPHandler.h"
#include "ServerUDP.h"

namespace LambdaEngine
{
	RemoteClientUDP::RemoteClientUDP(const std::string& address, uint16 port, uint64 hash, ServerUDP* server, IClientUDPHandler* handler) :
		m_Address(address),
		m_Port(port),
		m_Hash(hash),
		m_pServer(server),
		m_pHandler(handler)
	{

	}

	RemoteClientUDP::~RemoteClientUDP()
	{

	}

	bool RemoteClientUDP::SendPacket(NetworkPacket* packet)
	{
		return false;
	}

	bool RemoteClientUDP::SendPacketImmediately(NetworkPacket* packet)
	{
		return false;
	}

	void RemoteClientUDP::Release() 
	{
	}

	bool RemoteClientUDP::IsServerSide() const
	{
		return true;
	}

	const std::string& RemoteClientUDP::GetAddress() const
	{
		return m_Address;
	}

	uint16 RemoteClientUDP::GetPort() const
	{
		return m_Port;
	}

	int32 RemoteClientUDP::GetBytesSent() const
	{
		return int32();
	}

	int32 RemoteClientUDP::GetBytesReceived() const
	{
		return int32();
	}

	int32 RemoteClientUDP::GetPacketsSent() const
	{
		return int32();
	}

	int32 RemoteClientUDP::GetPacketsReceived() const
	{
		return int32();
	}

	uint64 RemoteClientUDP::GetHash() const
	{
		return m_Hash;
	}

	void RemoteClientUDP::OnPacketReceived(NetworkPacket* packet)
	{
		m_pHandler->OnClientPacketReceivedUDP(this, packet);
	}
}
