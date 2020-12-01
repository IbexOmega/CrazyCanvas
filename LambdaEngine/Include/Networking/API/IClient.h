#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IPEndPoint.h"
#include "Networking/API/PacketManagerBase.h"

#define CREATE_COLOR_UINT32(arr, r, g, b)  \
arr[0] = r; \
arr[1] = g; \
arr[2] = b; \

namespace LambdaEngine
{
	class NetworkSegment;
	class IPacketListener;
	class NetworkStatistics;
	class IClientRemoteHandler;

	enum EClientState
	{
		STATE_DISCONNECTED,
		STATE_DISCONNECTING,
		STATE_CONNECTING,
		STATE_CONNECTED
	};

	class LAMBDA_API IClient
	{
		friend class NetworkDebugger;

	public:
		DECL_INTERFACE(IClient);

		virtual void Disconnect(const std::string& reason) = 0;
		virtual void Release() = 0;
		virtual bool IsConnected() = 0;
		virtual bool SendUnreliable(NetworkSegment* pPacket) = 0;
		virtual bool SendReliable(NetworkSegment* pPacket, IPacketListener* pListener = nullptr) = 0;
		virtual const IPEndPoint& GetEndPoint() const = 0;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) = 0;
		virtual EClientState GetState() const = 0;
		virtual NetworkStatistics* GetStatistics() = 0;
		virtual PacketManagerBase* GetPacketManager() = 0;
		virtual const PacketManagerBase* GetPacketManager() const = 0;
		virtual IClientRemoteHandler* GetHandler() = 0;
		virtual uint64 GetUID() const = 0;
		virtual void SetTimeout(Timestamp time) = 0;
		virtual void ResetTimeout() = 0;

		template<class T>
		bool SendUnreliableStruct(const T& packet, uint16 packetType)
		{
			NetworkSegment* pSegment = GetFreePacket(packetType);
			if (pSegment)
			{
				pSegment->Write<T>(&packet);
				return SendUnreliable(pSegment);
			}
			return false;
		}

		template<class T>
		bool SendReliableStruct(const T& packet, uint16 packetType, IPacketListener* pListener = nullptr)
		{
			NetworkSegment* pSegment = GetFreePacket(packetType);
			if (pSegment)
			{
				pSegment->Write<T>(&packet);
				return SendReliable(pSegment, pListener);
			}
			return false;
		}

		static std::string StateToString(EClientState state)
		{
			switch (state)
			{
			case STATE_DISCONNECTED:	return "DISCONNECTED";
			case STATE_DISCONNECTING:	return "DISCONNECTING";
			case STATE_CONNECTING:		return "CONNECTING";
			case STATE_CONNECTED:		return "CONNECTED";
			};
			return "UNKNOWN";
		};

		static uint32 StateToColor(EClientState state)
		{
			uint32 color = UINT32_MAX;
			uint8* arr = (uint8*)&color;
			switch (state)
			{
			case STATE_DISCONNECTED:	CREATE_COLOR_UINT32(arr, 255, 0, 0); break;
			case STATE_DISCONNECTING:	CREATE_COLOR_UINT32(arr, 255, 255, 0); break;
			case STATE_CONNECTING:		CREATE_COLOR_UINT32(arr, 255, 255, 0); break;
			case STATE_CONNECTED:		CREATE_COLOR_UINT32(arr, 0, 255, 0); break;
			};
			return color;
		};
	};
}