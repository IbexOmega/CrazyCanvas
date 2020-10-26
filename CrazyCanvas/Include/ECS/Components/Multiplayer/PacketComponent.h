#pragma once

#include <type_traits>

#include "ECS/Component.h"

#include "Containers/TArray.h"
#include "Containers/TQueue.h"

#include "Math/Math.h"

#include "Multiplayer/Packet/Packet.h"
#include "Networking/API/NetworkSegment.h"


struct IPacketComponent
{
	friend class PacketTranscoderSystem;

	virtual ~IPacketComponent() = default;

private:
	virtual void* AddPacketReceived() = 0;
	virtual void ClearPacketsReceived() = 0;
	virtual void WriteSegment(LambdaEngine::NetworkSegment* pSegment, int32 networkUID) = 0;
	virtual uint16 GetSize() = 0;
	virtual uint16 GetPacketsToSendCount() = 0;
	virtual uint16 GetPacketType() = 0;
};

template<class T>
struct PacketComponent : public IPacketComponent
{
	friend class PacketType;
	static_assert(std::is_base_of<Packet, T>::value, "T must inherit from Packet!");
	DECL_COMPONENT(PacketComponent<T>);

public:
	/*
	* Returns the packets received. The order is guaranteed to be the same as when the SendPacket was called.
	*/
	const LambdaEngine::TArray<T>& GetPacketsReceived() const
	{
		return m_PacketsReceived;
	}

	/*
	* Puts a packet in the queue for dispatch over the network system.
	*/
	void SendPacket(const T& packet)
	{
		m_PacketsToSend.push(packet);
	}

private:
	virtual void* AddPacketReceived() override final
	{
		return &m_PacketsReceived.PushBack({});
	}

	virtual void ClearPacketsReceived() override final
	{
		m_PacketsReceived.Clear();
	}

	virtual void WriteSegment(LambdaEngine::NetworkSegment* pSegment, int32 networkUID) override final
	{
		T& packet = m_PacketsToSend.front();
		packet.NetworkUID = networkUID;
		pSegment->Write<T>(&packet);
		m_PacketsToSend.pop();
	}

	virtual uint16 GetSize() override final
	{
		return (uint16)sizeof(T);
	}

	virtual uint16 GetPacketsToSendCount() override final
	{
		return (uint16)m_PacketsToSend.size();
	}

	virtual uint16 GetPacketType() override final
	{
		return s_PacketType;
	}

private:
	LambdaEngine::TArray<T> m_PacketsReceived;
	LambdaEngine::TQueue<T> m_PacketsToSend;
	inline static uint16 s_PacketType = 0;
};