#pragma once

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine 
{
	class Sampler;
	class TextureView;
	class Sampler;
	class Buffer;
	class CommandList;
	class RenderGraph;
	class DeviceChild;

	struct ParticleChunk
	{
		uint32 Offset;
		uint32 Size;
	};

	struct ParticleEmitterInstance
	{
		bool			OneTime = false;
		glm::vec3		Position;
		glm::quat		Rotation;
		float			Angle = 90.f;
		float			Velocity;
		float			Acceleration;
		float			ElapTime = 0.f;
		float			LifeTime;
		float			ParticleRadius;
		uint32			DataIndex = 0;
		ParticleChunk	ParticleChunk;
		uint32			AtlasIndex = 0;
		uint32			TileIndex = 0;
		uint32			AnimationCount = 0;
		uint32			FirstAnimationIndex = 0;
	};

	struct SParticle
	{
		glm::mat4 Transform;
		glm::vec3 Velocity;
		float CurrentLife;
		glm::vec3 StartVelocity;
		float Radius;
		glm::vec3 Acceleration;
		uint32 TileIndex;
		glm::vec3 StartPosition;
		uint32 EmitterIndex;
	};

	struct SEmitter
	{
		glm::vec4 Color;
		float LifeTime;
		float Radius;
		uint32 AtlasIndex;
		uint32 AnimationCount;
		uint32 FirstAnimationIndex;
		uint32 padding0			= 0;
		uint32 padding1			= 0;
		uint32 padding2			= 0;
	};

	struct SAtlasInfo
	{
		float	TileFactorX = 0.f;
		float	TileFactorY = 0.f;
		uint32	RowCount	= 0;
		uint32	ColCount	= 0;
		uint32	AtlasIndex	= 0;
	};

	struct IndirectData
	{
		uint32	IndexCount		= 0;
		uint32	InstanceCount	= 0;
		uint32	FirstIndex		= 0;
		int32	VertexOffset	= 0;
		uint32	FirstInstance	= 0;
	};

	class ParticleManager
	{
	public:
		ParticleManager() = default;
		~ParticleManager() = default;

		void Init(uint32 maxParticleCapacity);
		void Release();

		void Tick(Timestamp deltaTime, uint32 modFrameIndex);

		void UpdateParticleEmitter(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp);

		void OnEmitterEntityAdded(Entity entity);
		void OnEmitterEntityRemoved(Entity entity);

		uint32 GetParticleCount() const { return m_Particles.GetSize();  }
		uint32 GetActiveEmitterCount() const { return m_IndirectData.GetSize();  }
		uint32 GetMaxParticleCount() const { return m_MaxParticleCount; }

		TArray<TextureView*>& GetAtlasTextureViews() { return m_AtlasTextureViews; }
		TArray<Sampler*>& GetAtlasSamplers() { return m_AtlasSamplers; }

		bool UpdateBuffers(CommandList* pCommandList);
		bool UpdateResources(RenderGraph* pRendergraph);

	private:
		bool CreateAtlasTextureInstance(GUID_Lambda atlasGUID, uint32 tileSize);

		bool CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance);
		bool CreateTubeParticleEmitter(ParticleEmitterInstance& emitterInstance);
		bool CopyDataToBuffer(CommandList* pCommandList, void* data, uint64 size, Buffer** pStagingBuffers, Buffer** pBuffer, FBufferFlags flags, const String& name);

		bool DeactivateEmitterEntity(const ParticleEmitterInstance& emitterInstance);

		bool FreeParticleChunk(ParticleChunk chunk);
		bool MergeParticleChunk(const ParticleChunk& chunk);

		void CleanBuffers();

	private:
		uint32								m_MaxParticleCount;
		uint32								m_ModFrameIndex;

		bool								m_DirtyParticleBuffer	= false;
		bool								m_DirtyVertexBuffer		= false;
		bool								m_DirtyIndexBuffer		= false;
		bool								m_DirtyTransformBuffer	= false;
		bool								m_DirtyEmitterBuffer	= false;
		bool								m_DirtyIndirectBuffer	= false;
		bool								m_DirtyAtlasDataBuffer	= false;

		Buffer*								m_ppIndirectStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pIndirectBuffer = nullptr;

		Buffer*								m_ppVertexStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pVertexBuffer = nullptr;

		Buffer*								m_ppIndexStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pIndexBuffer = nullptr;

		Buffer*								m_ppTransformStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pTransformBuffer = nullptr;

		Buffer*								m_ppEmitterStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pEmitterBuffer = nullptr;

		Buffer*								m_ppParticleStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pParticleBuffer = nullptr;

		Buffer*								m_ppAtlasDataStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pAtlasDataBuffer = nullptr;

		TArray<DeviceChild*>				m_ResourcesToRemove[BACK_BUFFER_COUNT];

		TArray<SParticle>					m_Particles;
		// Emitter specfic data
		TArray<IndirectData>				m_IndirectData;
		TArray<SEmitter>					m_EmitterData;
		TArray<glm::mat4>					m_EmitterTransformData;
		THashTable<uint32, Entity>			m_DataToEntity;

		TArray<ParticleChunk>				m_FreeParticleChunks;
		TArray<SAtlasInfo>					m_AtlasInfoData;

		TSharedRef<Sampler>					m_Sampler = nullptr;
		GUID_Lambda							m_DefaultAtlasTextureGUID;
		THashTable<GUID_Lambda, SAtlasInfo>	m_AtlasResources;
		TArray<TextureView*>				m_AtlasTextureViews;
		TArray<Sampler*>					m_AtlasSamplers;

		THashTable<Entity, ParticleEmitterInstance>	m_ActiveEmitters;
		THashTable<Entity, ParticleEmitterInstance>	m_SleepingEmitters;
	};
}