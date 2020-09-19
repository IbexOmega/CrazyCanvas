#include "States/BenchmarkState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"
#include "Utilities/RuntimeStats.h"

#include "Game/ECS/Systems/TrackSystem.h"

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

BenchmarkState::BenchmarkState()
{}

BenchmarkState::~BenchmarkState()
{
}

void BenchmarkState::Init()
{
	using namespace LambdaEngine;
	Input::Disable();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	TrackSystem::GetInstance().Init();

	std::vector<glm::vec3> cameraTrack = {
		{-2.0f, 1.6f, 1.0f},
		{9.8f, 1.6f, 0.8f},
		{9.4f, 1.6f, -3.8f},
		{-9.8f, 1.6f, -3.9f},
		{-11.6f, 1.6f, -1.1f},
		{9.8f, 6.1f, -0.8f},
		{9.4f, 6.1f, 3.8f},
		{-9.8f, 6.1f, 3.9f}
	};

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV");
	cameraDesc.Width = window->GetWidth();
	cameraDesc.Height = window->GetHeight();
	cameraDesc.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane");
	cameraDesc.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane");
	m_Camera = CreateCameraTrackEntity(cameraDesc, cameraTrack);

	// Load scene
	TArray<MeshComponent> meshComponents;
	ResourceManager::LoadSceneFromFile("Map/scene.obj", meshComponents);

	const glm::vec3 position(0.0f, 0.0f, 0.0f);
	const glm::vec3 scale(0.01f);

	ECSCore* pECS = ECSCore::GetInstance();
	for (const MeshComponent& meshComponent : meshComponents)
	{
		Entity entity = ECSCore::GetInstance()->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<StaticComponent>(entity, StaticComponent());
	}
}

void BenchmarkState::Tick(float dt)
{
	if (LambdaEngine::TrackSystem::GetInstance().HasReachedEnd(m_Camera))
	{
		PrintBenchmarkResults();
		LambdaEngine::CommonApplication::Get()->Terminate();
		return;
	}
}

void BenchmarkState::PrintBenchmarkResults()
{
	using namespace rapidjson;
	using namespace LambdaEngine;

	constexpr const float MB = 1000000.0f;

	StringBuffer jsonStringBuffer;
	PrettyWriter<StringBuffer> writer(jsonStringBuffer);

	writer.StartObject();

	writer.String("AverageFPS");
	writer.Double(1.0f / RuntimeStats::GetAverageFrametime());
	writer.String("PeakRAM");
	writer.Double(RuntimeStats::GetPeakMemoryUsage() / MB);

	writer.EndObject();

	FILE* pFile = fopen("benchmark_results.json", "w");

	if (pFile)
	{
		fputs(jsonStringBuffer.GetString(), pFile);
		fclose(pFile);
	}
}
