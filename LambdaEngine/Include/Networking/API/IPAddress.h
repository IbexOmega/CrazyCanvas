#pragma once

#include "Defines.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"
#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class LAMBDA_API IPAddress
	{
		friend class NetworkUtils;

	public:
		virtual ~IPAddress();

		/*
		* return - The ip-address as a string
		*/
		const std::string& ToString() const;

		uint64 GetHash() const;

	protected:
		IPAddress(const std::string& address, uint64 hash);

	public:

		/*
		* address - The ip-address as a string
		*
		* return  - The IPAddress* representing the given address
		*/
		static IPAddress* Get(const std::string& address);

	private:
		static void InitStatic();
		static void ReleaseStatic();

	private:
		std::string m_Address;
		uint64 m_Hash;

	public:
		static IPAddress* const ANY;
		static IPAddress* const BROADCAST;
		static IPAddress* const LOOPBACK;

	protected:
		static const char* const ADDRESS_ANY;
		static const char* const ADDRESS_BROADCAST;
		static const char* const ADDRESS_LOOPBACK;

	private:
		static std::unordered_map<uint64, IPAddress*> s_CachedAddresses;
		static SpinLock m_Lock;
		static bool s_Released;
	};
}
