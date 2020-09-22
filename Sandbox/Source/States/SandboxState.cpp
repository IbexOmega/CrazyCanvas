#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Engine/EngineConfig.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/TrackSystem.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Math/Random.h"

using namespace LambdaEngine;

SandboxState::SandboxState()
{

}

SandboxState::SandboxState(LambdaEngine::State* pOther) : LambdaEngine::State(pOther)
{
}

SandboxState::~SandboxState()
{
	// Remove System
}

void SandboxState::Init()
{
	// Create Systems
	TrackSystem::GetInstance().Init();
	ECSCore* pECS = ECSCore::GetInstance();

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		CameraDesc cameraDesc = {};
		cameraDesc.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV");
		cameraDesc.Width = window->GetWidth();
		cameraDesc.Height = window->GetHeight();
		cameraDesc.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane");
		cameraDesc.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane");
		Entity e = CreateFreeCameraEntity(cameraDesc);
	}

	//Scene
	{
		/*TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Testing/Testing.obj", meshComponents);

		glm::vec3 position(0.0f, 0.0f, 0.0f);
		glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 scale(1.0f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { position, true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { scale, true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}*/
	}

	// Load scene
	//{
	//	TArray<MeshComponent> meshComponents;
	//	ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

	//	const glm::vec3 position(0.0f, 0.0f, 0.0f);
	//	const glm::vec3 scale(0.01f);

	//	for (const MeshComponent& meshComponent : meshComponents)
	//	{
	//		Entity entity = ECSCore::GetInstance()->CreateEntity();
	//		pECS->AddComponent<PositionComponent>(entity, { position, true });
	//		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
	//		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
	//		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	//	}
	//}

	//BufferDesc testBufferDesc = { };
	//testBufferDesc.DebugName		= "Sandbox TEST Buffer";
	//testBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
	//testBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST;
	//testBufferDesc.SizeInBytes		= 64;

	//Buffer* pTestBuffer = RenderAPI::GetDevice()->CreateBuffer(&testBufferDesc);

	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		//CommandAllocator* pCommandAllocator = RenderAPI::GetDevice()->CreateCommandAllocator("Sandbox Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

		//CommandListDesc computeCopyCommandListDesc = {};
		//computeCopyCommandListDesc.DebugName		= "Sandbox Compute Command List";
		//computeCopyCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		//computeCopyCommandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		//CommandList* pComputeCommandList = RenderAPI::GetDevice()->CreateCommandList(pCommandAllocator, &computeCopyCommandListDesc);

		//pCommandAllocator->Reset();
		//pComputeCommandList->Begin(nullptr);

		//TArray<AccelerationStructureInstance> instances;

		//Mesh* pMesh = ResourceManager::GetMesh(sphereMeshGUID);

		//Buffer* pVertexBuffer;

		//{
		//	BufferDesc vertexStagingBufferDesc = { };
		//	vertexStagingBufferDesc.DebugName		= "Sandbox Staging Vertex Buffer";
		//	vertexStagingBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		//	vertexStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		//	vertexStagingBufferDesc.SizeInBytes		= pMesh->VertexCount * sizeof(Vertex);

		//	Buffer* pVertexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexStagingBufferDesc);

		//	void* pMapped = pVertexStagingBuffer->Map();
		//	memcpy(pMapped, pMesh->pVertexArray, vertexStagingBufferDesc.SizeInBytes);
		//	pVertexStagingBuffer->Unmap();

		//	BufferDesc vertexBufferDesc = { };
		//	vertexBufferDesc.DebugName		= "Sandbox Vertex Buffer";
		//	vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
		//	vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_RAY_TRACING | FBufferFlag::BUFFER_FLAG_COPY_DST;
		//	vertexBufferDesc.SizeInBytes	= pMesh->VertexCount * sizeof(Vertex);

		//	pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);

		//	pComputeCommandList->CopyBuffer(pVertexStagingBuffer, 0, pVertexBuffer, 0, vertexBufferDesc.SizeInBytes);
		//}

		//Buffer* pIndexBuffer;
		//{
		//	BufferDesc indexStagingBufferDesc = { };
		//	indexStagingBufferDesc.DebugName		= "Sandbox Staging Index Buffer";
		//	indexStagingBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		//	indexStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		//	indexStagingBufferDesc.SizeInBytes		= pMesh->IndexCount * sizeof(uint32);

		//	Buffer* pIndexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&indexStagingBufferDesc);

		//	void* pMapped = pIndexStagingBuffer->Map();
		//	memcpy(pMapped, pMesh->pIndexArray, indexStagingBufferDesc.SizeInBytes);
		//	pIndexStagingBuffer->Unmap();

		//	BufferDesc indexBufferDesc = { };
		//	indexBufferDesc.DebugName		= "Sandbox Staging Buffer";
		//	indexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
		//	indexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_RAY_TRACING | FBufferFlag::BUFFER_FLAG_COPY_DST;
		//	indexBufferDesc.SizeInBytes		= pMesh->IndexCount * sizeof(uint32);

		//	pIndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&indexBufferDesc);

		//	pComputeCommandList->CopyBuffer(pIndexStagingBuffer, 0, pIndexBuffer, 0, indexBufferDesc.SizeInBytes);
		//}

		//AccelerationStructureDesc blasCreateDesc = {};
		//blasCreateDesc.DebugName			= "Sandbox BLAS";
		//blasCreateDesc.Type					= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
		//blasCreateDesc.Flags				= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		//blasCreateDesc.MaxTriangleCount		= pMesh->IndexCount / 3;
		//blasCreateDesc.MaxVertexCount		= pMesh->VertexCount;
		//blasCreateDesc.AllowsTransform		= false;

		//AccelerationStructure* pBLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&blasCreateDesc);

		//BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
		//blasBuildDesc.pAccelerationStructure	= pBLAS;
		//blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		//blasBuildDesc.pVertexBuffer				= pVertexBuffer;
		//blasBuildDesc.FirstVertexIndex			= 0;
		//blasBuildDesc.VertexStride				= sizeof(Vertex);
		//blasBuildDesc.pIndexBuffer				= pIndexBuffer;
		//blasBuildDesc.IndexBufferByteOffset		= 0;
		//blasBuildDesc.TriangleCount				= pMesh->IndexCount / 3;
		//blasBuildDesc.pTransformBuffer			= nullptr;
		//blasBuildDesc.TransformByteOffset		= 0;
		//blasBuildDesc.Update					= false;

		//pComputeCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);

		uint32 gridRadius = 5;

		for (uint32 y = 0; y < gridRadius; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness = roughness;
				materialProperties.Metallic = metallic;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				glm::vec3 position(-float32(gridRadius) * 0.5f + x, 1.0f + y, 5.0f);
				glm::vec3 scale(1.0f);

				Entity entity = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(entity, { position, true });
				pECS->AddComponent<ScaleComponent>(entity, { scale, true });
				pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
				pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);


				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
				transform *= glm::toMat4(glm::identity<glm::quat>());
				transform = glm::scale(transform, scale);

				//AccelerationStructureInstance asInstance = {};
				//asInstance.Transform		= glm::transpose(transform);
				//asInstance.CustomIndex		= 0;
				//asInstance.Mask				= 0xFF;
				//asInstance.SBTRecordOffset	= 0;
				//asInstance.Flags			= RAY_TRACING_INSTANCE_FLAG_CULLING_DISABLED;
				//asInstance.AccelerationStructureAddress = pBLAS->GetDeviceAdress();
				//instances.PushBack(asInstance);
			}
		}

		//Buffer* pInstanceBuffer;
		//{
		//	BufferDesc instanceStagingBufferDesc = { };
		//	instanceStagingBufferDesc.DebugName			= "Sandbox Staging Instance Buffer";
		//	instanceStagingBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		//	instanceStagingBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		//	instanceStagingBufferDesc.SizeInBytes		= instances.GetSize() * sizeof(AccelerationStructureInstance);

		//	Buffer* pInstanceStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceStagingBufferDesc);

		//	void* pMapped = pInstanceStagingBuffer->Map();
		//	memcpy(pMapped, instances.GetData(), instanceStagingBufferDesc.SizeInBytes);
		//	pInstanceStagingBuffer->Unmap();

		//	BufferDesc instanceBufferDesc = { };
		//	instanceBufferDesc.DebugName		= "Sandbox Instance Buffer";
		//	instanceBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
		//	instanceBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_RAY_TRACING | FBufferFlag::BUFFER_FLAG_COPY_DST;
		//	instanceBufferDesc.SizeInBytes		= instances.GetSize() * sizeof(AccelerationStructureInstance);

		//	pInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&instanceBufferDesc);

		//	pComputeCommandList->CopyBuffer(pInstanceStagingBuffer, 0, pInstanceBuffer, 0, instanceBufferDesc.SizeInBytes);
		//}

		//AccelerationStructureDesc tlasCreateDesc = {};
		//tlasCreateDesc.DebugName			= "Sandbox TLAS";
		//tlasCreateDesc.Type					= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
		//tlasCreateDesc.Flags				= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		//tlasCreateDesc.InstanceCount		= instances.GetSize();

		//AccelerationStructure* pTLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&tlasCreateDesc);

		//BuildTopLevelAccelerationStructureDesc tlasBuildDesc = {};
		//tlasBuildDesc.pAccelerationStructure	= pTLAS;
		//tlasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		//tlasBuildDesc.pInstanceBuffer			= pInstanceBuffer;
		//tlasBuildDesc.InstanceCount				= instances.GetSize();
		//tlasBuildDesc.Update					= false;

		//pComputeCommandList->BuildTopLevelAccelerationStructure(&tlasBuildDesc);

		//pComputeCommandList->End();

		//RenderAPI::GetComputeQueue()->ExecuteCommandLists(&pComputeCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
		//RenderAPI::GetComputeQueue()->Flush();

		//ResourceUpdateDesc resourceUpdateDesc = { };
		//resourceUpdateDesc.ResourceName							= SCENE_TLAS;
		//resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

		//RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&resourceUpdateDesc);

		// Directional Light
	/*	{
			m_DirLight = ECSCore::GetInstance()->CreateEntity();
			ECSCore::GetInstance()->AddComponent<RotationComponent>(m_DirLight, { glm::quatLookAt({1.0f, -1.0f, 0.0f}, g_DefaultUp), true });
			ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(m_DirLight, DirectionalLightComponent{ .ColorIntensity = {1.0f, 1.0f, 1.0f, 5.0f} });
		}*/
		
		// Add PointLights
		/*{
			constexpr uint32 POINT_LIGHT_COUNT = 3;
			const PointLightComponent pointLights[POINT_LIGHT_COUNT] =
			{
				{.ColorIntensity = {1.0f, 0.0f, 0.0f, 25.0f}},
				{.ColorIntensity = {0.0f, 1.0f, 0.0f, 25.0f}},
				{.ColorIntensity = {0.0f, 0.0f, 1.0f, 25.0f}},
			};

			const glm::vec3 startPosition[3] =
			{
				{4.0f, 0.0f, 5.0f},
				{-4.0f, 0.0f, 5.0f},
				{0.0f, 0.0f, 6.0f},
			};

			const float PI = glm::pi<float>();
			const float RADIUS = 3.0f;
			for (uint32 i = 0; i < 3; i++)
			{
				float positive = std::pow(-1.0, i);


				MaterialProperties materialProperties;
				glm::vec3 color = pointLights[i].ColorIntensity;
				materialProperties.Albedo = glm::vec4(color, 1.0f);
				materialProperties.Roughness = 0.1f;
				materialProperties.Metallic = 0.1f;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				m_PointLights[i] = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(m_PointLights[i], { startPosition[i], true });
				pECS->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
				pECS->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
				pECS->AddComponent<PointLightComponent>(m_PointLights[i], pointLights[i]);
				pECS->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
			}
		}*/

		//{
		//	constexpr uint32 POINT_LIGHT_COUNT = 30;

		//	const float PI = glm::pi<float>();
		//	const float RADIUS = 3.0f;
		//	for (uint32 i = 0; i < POINT_LIGHT_COUNT; i++)
		//	{
		//		TArray<glm::vec3> lightPath;
		//		lightPath.Reserve(36);
		//		float positive = std::pow(-1.0, i);

		//		PointLightComponent ptComp =
		//		{
		//			.ColorIntensity = glm::vec4(Random::Float32(0.0f, 1.0f), Random::Float32(0.0f, 1.0f), Random::Float32(0.0f, 1.0f), Random::Float32(1.0f, 10.0f))
		//		};

		//		glm::vec3 startPosition(0.0f, 0.0f, 5.0f + Random::Float32(-1.0f, 1.0f));
		//		for (uint32 y = 0; y < 6; y++)
		//		{
		//			float angle = 0.f;
		//			for (uint32 x = 0; x < 6; x++)
		//			{
		//				glm::vec3 position = startPosition;
		//				angle += positive * (2.0f * PI / 6.0f);
		//				position.x += std::cos(angle) * RADIUS;
		//				position.z += std::sin(angle) * RADIUS;
		//				position.y += 1.0f + y * Random::Float32(0.5f, 1.0f) + i * 0.1f;
		//				lightPath.PushBack(position);
		//			}
		//		}

		//		MaterialProperties materialProperties;
		//		glm::vec3 color = ptComp.ColorIntensity;
		//		materialProperties.Albedo = glm::vec4(color, 1.0f);
		//		materialProperties.Roughness = 0.1f;
		//		materialProperties.Metallic = 0.1f;

		//		MeshComponent sphereMeshComp = {};
		//		sphereMeshComp.MeshGUID = sphereMeshGUID;
		//		sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
		//			"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
		//			GUID_TEXTURE_DEFAULT_COLOR_MAP,
		//			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		//			GUID_TEXTURE_DEFAULT_COLOR_MAP,
		//			GUID_TEXTURE_DEFAULT_COLOR_MAP,
		//			GUID_TEXTURE_DEFAULT_COLOR_MAP,
		//			materialProperties);

		//		m_PointLights[i] = pECS->CreateEntity();
		//		pECS->AddComponent<PositionComponent>(m_PointLights[i], { {0.0f, 0.0f, 0.0f}, true });
		//		pECS->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
		//		pECS->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
		//		pECS->AddComponent<PointLightComponent>(m_PointLights[i], ptComp);
		//		//pECS->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
		//		pECS->AddComponent<TrackComponent>(m_PointLights[i], TrackComponent{ .Track = lightPath });
		//	}
		//}
	}

	//Mirrors
	/*{
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

		constexpr const uint32 NUM_MIRRORS = 6;
		for (uint32 i = 0; i < NUM_MIRRORS; i++)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();

			float32 sign = pow(-1.0f, i % 2);
			pECS->AddComponent<PositionComponent>(entity, { glm::vec3(3.0f * (float32(i / 2) - float32(NUM_MIRRORS) / 2.0f), 2.0f, 1.5f * sign), true });
			pECS->AddComponent<RotationComponent>(entity, { glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(-sign * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f))), true });
			pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}
	}*/

	// Load Scene SceneManager::Get("SceneName").Load()

	// Use HelperClass to create additional entities

	// EntityIndex index = HelperClass::CreatePlayer(
}

void SandboxState::Resume()
{
	// Unpause System

	// Reload Page
}

void SandboxState::Pause()
{
	// Pause System

	// Unload Page
}

void SandboxState::Tick(LambdaEngine::Timestamp delta)
{
	// Update State specfic objects
}
