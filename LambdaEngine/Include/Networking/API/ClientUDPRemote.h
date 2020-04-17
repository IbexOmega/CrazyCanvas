#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClient.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	class ServerUDP;

	class LAMBDA_API ClientUDPRemote : public IClient
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
		ClientUDPRemote(uint16 packets, const IPEndPoint& ipEndPoint, ServerUDP* pServer);

	private:
		PacketManager* GetPacketManager();
		void OnDataReceived(const char* data, int32 size);
		void SendPackets(char* data);

	private:
		ServerUDP* m_pServer;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
	};
}