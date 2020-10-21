#pragma once

#include "ECS/Component.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

struct IPacketComponent
{
	friend class PacketDecoderSystem;

private:
	virtual void* AddPacketReceived() = 0;
	virtual void ClearPacketsReceived() = 0;
	virtual uint16 GetSize() = 0;
};

template<typename PacketType>
struct PacketComponent : public IPacketComponent
{
	DECL_COMPONENT(PacketComponent<PacketType>);
	LambdaEngine::TArray<PacketType> PacketsReceived;
	LambdaEngine::TArray<PacketType> PacketsToSend;

private:
	virtual void* AddPacketReceived() override final
	{
		return &PacketsReceived.PushBack({});
	}

	virtual void ClearPacketsReceived() override final
	{
		PacketsReceived.Clear();
	}

	virtual uint16 GetSize() override final
	{
		return sizeof(PacketType);
	}
};

#pragma pack(push, 1)
struct PlayerAction
{
	int32 SimulationTick = -1;
	glm::quat Rotation;
	int8 DeltaForward = 0;
	int8 DeltaLeft = 0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PlayerActionResponse
{
	int32 SimulationTick = -1;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::quat Rotation;
};
#pragma pack(pop)