#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	class ServerUDP;
	class IClientUDPHandler;

	class LAMBDA_API ClientUDPRemote : public IClientUDP
	{
		friend class ServerUDP;
		
	public:
		~ClientUDPRemote();

		virtual bool Connect(const IPEndPoint& ipEndPoint) override;
		virtual void Disconnect() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket() override;

	protected:
		ClientUDPRemote(uint16 packets, const IPEndPoint& ipEndPoint, IClientUDPHandler* pHandler, ServerUDP* pServer);

	private:
		PacketManager* GetPacketManager();
		void OnDataReceived(const char* data, int32 size);
		void SendPackets(char* data);

	private:
		ServerUDP* m_pServer;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		NetworkPacket* m_pPackets[32];
		IClientUDPHandler* m_pHandler;
	};
}