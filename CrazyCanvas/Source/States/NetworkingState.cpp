#include "States/NetworkingState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"

#include "Game/ECS/Systems/Networking/Client/ClientSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"

#include "World/Level.h"
#include "World/LevelManager.h"

#include "Application/API/Events/EventQueue.h"

NetworkingState::~NetworkingState()
{
	SAFEDELETE(m_pLevel);
}

void NetworkingState::Init()
{
	using namespace LambdaEngine;

	ClientSystem& clientSystem = ClientSystem::GetInstance();
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &NetworkingState::OnPacketReceived);
	
	ECSCore* pECS = ECSCore::GetInstance();

	// Load scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
		MultiplayerUtils::RegisterClientEntityAccessor(m_pLevel);
	}

	clientSystem.Connect(NetworkUtils::GetLocalAddress());
}

bool NetworkingState::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	using namespace LambdaEngine;

	if (event.Type == NetworkSegment::TYPE_ENTITY_CREATE)
	{
		BinaryDecoder decoder(event.pPacket);
		bool isLocal = decoder.ReadBool();
		int32 networkUID = decoder.ReadInt32();
		glm::vec3 position = decoder.ReadVec3();

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

		const CameraDesc cameraDesc =
		{
			.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV"),
			.Width = (float)window->GetWidth(),
			.Height = (float)window->GetHeight(),
			.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane"),
			.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane")
		};

		TArray<GUID_Lambda> animations;
		const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f);
		materialProperties.Roughness = 1.0f;
		materialProperties.Metallic = 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID = robotGUID;
		robotMeshComp.MaterialGUID = robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;
		robotAnimationComp.AnimationGUID = animations[0];

		CreatePlayerDesc createPlayerDesc =
		{
			.IsLocal = isLocal,
			.NetworkUID = networkUID,
			.pClient = event.pClient,
			.Position = position,
			.Forward = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
			.Scale = glm::vec3(1.0f),
			.pCameraDesc = &cameraDesc,
			.MeshComponent = robotMeshComp,
			.AnimationComponent = robotAnimationComp,
		};

		m_pLevel->CreateObject(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER, &createPlayerDesc);

		return true;
	}

	return false;
}

void NetworkingState::Tick(LambdaEngine::Timestamp)
{

}
