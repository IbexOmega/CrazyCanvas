#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Networking/API/IPacketListener.h"

class ClientHelper
{
	DECL_STATIC_CLASS(ClientHelper);

public:
	template<class T>
	static bool SendUnreliable(const T& packet);

	template<class T>
	static bool SendReliable(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);
};

template<class T>
static bool ClientHelper::SendUnreliable(const T& packet)
{
	LambdaEngine::IClient* pClient = LambdaEngine::ClientSystem::GetInstance().GetClient();
	return pClient->SendUnreliable<T>(packet, T::Type());
}

template<class T>
static bool ClientHelper::SendReliable(const T& packet, LambdaEngine::IPacketListener* pListener)
{
	LambdaEngine::IClient* pClient = LambdaEngine::ClientSystem::GetInstance().GetClient();
	return pClient->SendReliable<T>(packet, T::Type(), pListener);
}