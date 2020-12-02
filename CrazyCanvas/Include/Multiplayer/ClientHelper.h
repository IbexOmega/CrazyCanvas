#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/IPAddress.h"

#include "Time/API/Timestamp.h"

class ClientHelper
{
	DECL_STATIC_CLASS(ClientHelper);

public:
	template<class T>
	static bool Send(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	static void AddNetworkDiscoveryTarget(LambdaEngine::IPAddress* pAddress);
	static void RemoveNetworkDiscoveryTarget(LambdaEngine::IPAddress* pAddress);
	static void Disconnect(const LambdaEngine::String& reason);

	static void SetTimeout(LambdaEngine::Timestamp time);
	static void ResetTimeout();
};

template<class T>
bool ClientHelper::Send(const T& packet, LambdaEngine::IPacketListener* pListener)
{
	ASSERT_MSG(T::GetType() != 0, "Packet type not registered!")
	LambdaEngine::IClient* pClient = LambdaEngine::ClientSystem::GetInstance().GetClient();
	return pClient->SendReliableStruct<T>(packet, T::GetType(), pListener);
}