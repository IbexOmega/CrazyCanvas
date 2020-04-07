#pragma once

#include "IClientUDP.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IClientUDPHandler;
	class ServerUDP;

	class LAMBDA_API ClientUDPRemote : public IClientUDP
	{
		friend class ServerUDP;

	public:
		ClientUDPRemote(const std::string& address, uint16 port, uint64 hash, ServerUDP* server, IClientUDPHandler* handler);
		~ClientUDPRemote();

		/*
		* Start the client and set the given ip-address and port. To send to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Start(const std::string& address, uint16 port) override final;

		/*
		* Sends a packet
		*/
		virtual bool SendPacket(NetworkPacket* packet) override final;

		/*
		* Sends a packet Immediately using the current thread
		*/
		virtual bool SendPacketImmediately(NetworkPacket* packet) override final;

		/*
		* Release all the resouces used by the client and will be deleted when each thread has terminated.
		*/
		virtual void Release() override final;

		/*
		* return - true if this instance is on the server side, otherwise false.
		*/
		virtual bool IsServerSide() const override final;

		/*
		* return - The currently used inet address.
		*/
		virtual const std::string& GetAddress() const override final;

		/*
		* return - The currently used port.
		*/
		virtual uint16 GetPort() const override final;

		/*
		* return - The total number of bytes sent
		*/
		virtual int32 GetBytesSent() const override final;

		/*
		* return - The total number of bytes received
		*/
		virtual int32 GetBytesReceived() const override final;

		/*
		* return - The total number of packets sent
		*/
		virtual int32 GetPacketsSent() const override final;

		/*
		* return - The total number of packets received
		*/
		virtual int32 GetPacketsReceived() const override final;

		/*
		* return - The hash representing this client
		*/
		uint64 GetHash() const;

	private:
		void OnPacketReceived(NetworkPacket* packet);

	private:
		std::string m_Address;
		uint16 m_Port;
		uint64 m_Hash;
		IClientUDPHandler* m_pHandler;
		ServerUDP* m_pServer;
	};
}