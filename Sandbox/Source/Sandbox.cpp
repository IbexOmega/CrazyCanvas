#include "Sandbox.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/SocketFactory.h"

Sandbox::Sandbox()
{
	using namespace LambdaEngine;

#ifdef LAMBDA_PLATFORM_WINDOWS

	//TCP TEST
	ISocketTCP* server = SocketFactory::CreateSocketTCP();
	server->Bind("127.0.0.1", 4444);
	server->Listen();

	ISocketTCP* client = SocketFactory::CreateSocketTCP();
	client->Connect("127.0.0.1", 4444);

	ISocketTCP* serverClient = server->Accept();

	std::string data = "Hello Guy!";
	uint32 bytesSent;
	serverClient->Send(data.c_str(), data.length(), bytesSent);

	char buffer[256];
	uint32 bytesReceived;
	client->Receive(buffer, 256, bytesReceived);

	LOG_MESSAGE(buffer);
	serverClient->Close();
	client->Close();
	server->Close();


	//UDP TEST
	ISocketUDP* socket1 = SocketFactory::CreateSocketUDP();
	ISocketUDP* socket2 = SocketFactory::CreateSocketUDP();

	socket2->Bind("127.0.0.1", 4444);
	socket1->SendTo(data.c_str(), data.length(), bytesSent, "127.0.0.1", 4444);

	buffer[256];
	std::string sender;
	uint16 port;
	socket2->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);

	data = "Vafan Guy!";
	socket2->SendTo(data.c_str(), data.length(), bytesSent, sender, port);
	socket1->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);
	socket1->Close();
	socket2->Close();


	//UDP Broadcast TEST
	LOG_MESSAGE("Broadcast");
	socket1 = SocketFactory::CreateSocketUDP();
	socket2 = SocketFactory::CreateSocketUDP();

	socket1->EnableBroadcast();
	socket2->EnableBroadcast();

	socket1->Bind("", 4444);

	data = "Ny data Guy!";

	socket2->Broadcast(data.c_str(), data.length(), bytesSent, 4444);

	socket1->ReceiveFrom(buffer, 256, bytesReceived, sender, port);
	LOG_MESSAGE(buffer);
	LOG_MESSAGE(sender.c_str());
	LOG_MESSAGE("%d", port);

#endif
}

Sandbox::~Sandbox()
{
}

void Sandbox::OnKeyDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Pressed: %d", key);
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	LOG_MESSAGE("Mouse Scrolled: %d", delta);
}

void Sandbox::Tick()
{
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = new Sandbox();
        Input::AddKeyboardHandler(pSandbox);
        Input::AddMouseHandler(pSandbox);
        
        return pSandbox;
    }
}
