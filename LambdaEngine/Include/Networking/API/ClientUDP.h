#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	class ISocketUDP;

	class LAMBDA_API ClientUDP : public NetWorker, public IClientUDP
	{
	public:
		~ClientUDP();

		virtual bool Connect(const IPEndPoint& ipEndPoint) override;
		virtual void Disconnect() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket() override;

	protected:
		ClientUDP(uint16 packets);

		virtual bool OnThreadsStarted() override;
		virtual void RunTranmitter() override;
		virtual void RunReceiver() override;
		virtual void OnThreadsTurminated() override;
		virtual void OnTerminationRequested() override;
		virtual void OnReleaseRequested() override;

	private:
		ISocketUDP* m_pSocket;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;

	public:
		static ClientUDP* Create(uint16 packets);
	};
}