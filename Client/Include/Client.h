#pragma once

#include "Game/Game.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryClient.h"

namespace LambdaEngine
{
	class IClient;
	class NetworkSegment;
	class ClientBase;
}

class Client :
	public LambdaEngine::Game,
	public LambdaEngine::INetworkDiscoveryClient
{
public:
	Client();
	~Client();

	virtual void OnServerFound(const LambdaEngine::BinaryDecoder& decoder, const LambdaEngine::IPEndPoint& endPoint) override;

	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	bool LoadRendererResources();

private:

	GUID_Lambda m_MeshSphereGUID;
};
