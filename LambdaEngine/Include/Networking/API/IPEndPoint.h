#pragma once

#include "LambdaEngine.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	class IPAddress;

	class LAMBDA_API IPEndPoint
	{
	public:
		IPEndPoint();
		IPEndPoint(IPAddress* pAddress, uint16 port);
		~IPEndPoint();

		void SetEndPoint(IPAddress* pAddress, uint16 port);

		void SetAddress(IPAddress* pAddress);

		IPAddress* GetAddress() const;

		void SetPort(uint16 port);

		uint16 GetPort() const;

		std::string ToString() const;

		uint64 GetHash() const;

		bool operator==(const IPEndPoint& other) const;

	private:
		void UpdateHash();

	private:
		IPAddress* m_pAddress;
		uint16 m_Port;
		uint64 m_Hash;
	};

	struct IPEndPointHasher
	{
		size_t operator()(const IPEndPoint& key) const
		{
			return key.GetHash();
		}
	};
}
