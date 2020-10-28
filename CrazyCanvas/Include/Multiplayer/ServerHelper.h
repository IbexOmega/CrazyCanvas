#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Networking/API/IPacketListener.h"

class ServerHelper
{
	DECL_STATIC_CLASS(ServerHelper);

public:
	template<class T>
	static bool Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	template<class T>
	static bool SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr, LambdaEngine::IClient* pExcludeClient = nullptr);
};

template<class T>
bool ServerHelper::Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener)
{
	return pClient->SendReliableStruct<T>(packet, T::Type(), pListener);
}

template<class T>
bool ServerHelper::SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener, LambdaEngine::IClient* pExcludeClient)
{
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();
	return pServer->SendReliableStructBroadcast<T>(packet, T::Type(), pListener, pExcludeClient);
}