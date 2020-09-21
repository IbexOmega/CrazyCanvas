#include "States/BenchmarkState.h"

#include "Application/API/CommonApplication.h"
#include "Debug/GPUProfiler.h"
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

void BenchmarkState::Init()
{
	using namespace LambdaEngine;
	Input::Disable();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	TrackSystem::GetInstance().Init();

	TArray<glm::vec3> cameraTrack = {
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

	ECSCore* pECS = ECSCore::GetInstance();

	// Load scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

		const glm::vec3 position(0.0f, 0.0f, 0.0f);
		const glm::vec3 scale(0.01f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { position, true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { scale, true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}
	}

	//Mirrors
	{
		MaterialProperties mirrorProperties = {};
		mirrorProperties.Roughness = 0.0f;

		MeshComponent meshComponent;
		meshComponent.MeshGUID = GUID_MESH_QUAD;
		meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Mirror Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			mirrorProperties);

		constexpr const uint32 NUM_MIRRORS = 10;
		for (uint32 i = 0; i < NUM_MIRRORS; i++)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();

			float32 sign = pow(-1.0f, i % 2);
			pECS->AddComponent<PositionComponent>(entity, { glm::vec3(3.0f * (float32(i / 2) - float32(NUM_MIRRORS) / 4.0f), 2.0f, 1.5f * sign), true });
			pECS->AddComponent<RotationComponent>(entity, { glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(-sign * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f))), true });
			pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}
	}
}

void BenchmarkState::Tick(LambdaEngine::Timestamp delta)
{
	LambdaEngine::GPUProfiler::Get()->Tick(delta);

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

	const GPUProfiler* pGPUProfiler = GPUProfiler::Get();

	StringBuffer jsonStringBuffer;
	PrettyWriter<StringBuffer> writer(jsonStringBuffer);

	writer.StartObject();

	writer.String("AverageFPS");
	writer.Double(1.0f / RuntimeStats::GetAverageFrametime());
	writer.String("PeakRAM");
	writer.Double(RuntimeStats::GetPeakMemoryUsage() / MB);
	writer.String("PeakVRAM");
	writer.Double(pGPUProfiler->GetPeakDeviceMemory() / MB);
	writer.String("AverageVRAM");
	writer.Double(pGPUProfiler->GetAverageDeviceMemory() / MB);

	writer.EndObject();

	FILE* pFile = fopen("benchmark_results.json", "w");

	if (pFile)
	{
		fputs(jsonStringBuffer.GetString(), pFile);
		fclose(pFile);
	}
}
