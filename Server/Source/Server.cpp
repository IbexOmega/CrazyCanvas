#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Server::Server() :
	m_Dispatcher(512)
{
	using namespace LambdaEngine;

	m_pSocketUDP = PlatformNetworkUtils::CreateSocketUDP();
	m_pSocketUDP->Bind(IPEndPoint(IPAddress::ANY, 4444));

	Thread::Create(std::bind(&Server::Run, this), std::bind(&Server::Terminated, this));

	UpdateTitle();

	char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	char m_pReceiveBuffer[UINT16_MAX];
}

Server::~Server()
{
	delete m_pSocketUDP;
}

LambdaEngine::IPEndPoint g_Sender(LambdaEngine::IPAddress::NONE, 0);

void Server::OnPacketReceived(LambdaEngine::NetworkPacket* packet, const LambdaEngine::IPEndPoint& sender)
{
	using namespace LambdaEngine;

	BinaryDecoder decoder(packet);
	LOG_MESSAGE(decoder.ReadString().c_str());

	g_Sender = sender;
	NetworkPacket* response = m_Dispatcher.GetFreePacket();
	BinaryEncoder encoder(response);
	encoder.WriteString("I got your message");
	m_Dispatcher.EnqueuePacket(response);
}

void Server::Run()
{
	using namespace LambdaEngine;

	int32 bytesReceived = 0;
	int32 packetsReceived = 0;
	NetworkPacket* packets[32];
	IPEndPoint sender(IPAddress::NONE, 0);

	while (m_pSocketUDP->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, bytesReceived, sender))
	{
		if (m_Dispatcher.DecodePackets(m_pReceiveBuffer, bytesReceived, packets, packetsReceived))
		{
			for (int i = 0; i < packetsReceived; i++)
			{
				OnPacketReceived(packets[i], sender);
			}
			m_Dispatcher.Free(packets, packetsReceived);
		}
	}
}

void Server::Terminated()
{

}

void Server::OnKeyDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;
	PlatformApplication::Get()->GetWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");
}

void Server::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::FixedTick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;
    UNREFERENCED_VARIABLE(delta);

	int32 bytesWritten = 0;
	int32 bytesSent = 0;
	bool done = false;
	while (!done)
	{
		done = m_Dispatcher.EncodePackets(m_pSendBuffer, bytesWritten);
		if (bytesWritten > 0)
		{
			m_pSocketUDP->SendTo(m_pSendBuffer, bytesWritten, bytesSent, g_Sender);
		}
	}
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Server* pSandbox = DBG_NEW Server();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}