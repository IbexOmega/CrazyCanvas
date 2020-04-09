#include "Network/API/GameServer.h"
#include "Network/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	GameServer::GameServer() : 
		m_PacketBufferIndex(0)
	{
		m_PacketBuffers[0] = new std::queue<NetworkPacket*>();
		m_PacketBuffers[1] = new std::queue<NetworkPacket*>();
	}

	GameServer::~GameServer()
	{
		delete m_PacketBuffers[0];
		delete m_PacketBuffers[1];
	}

	bool GameServer::AddPacket(NetworkPacket* packet)
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketBuffers);
		m_PacketBuffers[m_PacketBufferIndex]->push(packet);
		return true;
	}

	void GameServer::RunTranmitter()
	{
		while (!ShouldTerminate())
		{

		}
	}

	void GameServer::RunReceiver()
	{
		while (!ShouldTerminate())
		{

		}
	}

	void GameServer::OnThreadsTurminated()
	{

	}

	std::queue<NetworkPacket*>* GameServer::SwapAndGetPacketBuffer()
	{
		std::scoped_lock<SpinLock> lock(m_LockPacketBuffers);
		std::queue<NetworkPacket*>* buffer = m_PacketBuffers[m_PacketBufferIndex];
		m_PacketBufferIndex = m_PacketBufferIndex % 2;
		return buffer;
	}
}