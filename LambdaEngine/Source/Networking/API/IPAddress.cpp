#include "Networking/API/IPAddress.h"

#include "Log/Log.h"

#include "Networking/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	std::unordered_map<uint64, IPAddress*> IPAddress::s_CachedAddresses;
	bool IPAddress::s_Released;
	SpinLock IPAddress::m_Lock;

	const char* const IPAddress::ADDRESS_ANY		= "ANY";
	const char* const IPAddress::ADDRESS_BROADCAST	= "BROADCAST";
	const char* const IPAddress::ADDRESS_LOOPBACK	= "LOOPBACK";

	IPAddress* const IPAddress::NONE		= Get("");
	IPAddress* const IPAddress::ANY			= Get(ADDRESS_ANY);
	IPAddress* const IPAddress::BROADCAST	= Get(ADDRESS_BROADCAST);
	IPAddress* const IPAddress::LOOPBACK	= Get(ADDRESS_LOOPBACK);

	IPAddress::IPAddress(const std::string& address, uint64 hash) :
		m_Address(address),
		m_Hash(hash)
	{
		
	}

	IPAddress::~IPAddress()
	{
		if (!s_Released)
		{
			LOG_ERROR("[IPAddress]: Manual delete on an IPAddress object is not allowed!");
		}
	}

	const std::string& IPAddress::ToString() const
	{
		return m_Address;
	}

	uint64 IPAddress::GetHash() const
	{
		return m_Hash;
	}

	bool IPAddress::operator==(const IPAddress& other) const
	{
		return m_Hash == other.m_Hash;
	}

	bool IPAddress::operator!=(const IPAddress& other) const
	{
		return m_Hash != other.m_Hash;
	}

	IPAddress* IPAddress::Get(const std::string& address)
	{
		uint64 hash = std::hash<std::string>{}(address);

		std::scoped_lock<SpinLock> lock(m_Lock);
		auto iterator = s_CachedAddresses.find(hash);
		if (iterator != s_CachedAddresses.end())
			return iterator->second;

		IPAddress* newAddress = PlatformNetworkUtils::CreateIPAddress(address, hash);
		s_CachedAddresses.insert({ newAddress->GetHash(), newAddress });
		return newAddress;
	}

	void IPAddress::InitStatic()
	{
		s_Released = false;
	}

	void IPAddress::ReleaseStatic()
	{
		s_Released = true;

		for (auto pair : s_CachedAddresses)
		{
			delete pair.second;
		}
		s_CachedAddresses.clear();
	}
}

namespace std
{
	template<>
	struct hash<LambdaEngine::IPAddress>
	{
		size_t operator()(LambdaEngine::IPAddress const& ipAddress) const
		{
			return ipAddress.GetHash();
		}
	};
}