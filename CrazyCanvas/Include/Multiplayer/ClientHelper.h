#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Networking/API/IPacketListener.h"

class ClientHelper
{
	DECL_STATIC_CLASS(ClientHelper);

public:
	template<class T>
	static bool Send(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);
};

template<class T>
bool ClientHelper::Send(const T& packet, LambdaEngine::IPacketListener* pListener)
{
	LambdaEngine::IClient* pClient = LambdaEngine::ClientSystem::GetInstance().GetClient();
	return pClient->SendReliableStruct<T>(packet, T::Type(), pListener);
}