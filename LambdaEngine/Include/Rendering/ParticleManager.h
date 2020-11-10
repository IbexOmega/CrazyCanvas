#pragma once

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"

#include "Rendering/Core/API/GraphicsTypes.h"

#include "Rendering/RT/ASBuilder.h"

#define DEBUG_PARTICLE false

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
		float			Explosive = 0.f;
		float			SpawnDelay; // This will be used if not explosive.
		glm::vec3		Position;
		glm::quat		Rotation;
		float			Angle = 90.f;
		float			Velocity;
		float			VelocityRandomness;
		float			Gravity;
		float			Acceleration;
		float			AccelerationRandomness;
		float			ElapTime = 0.f;
		float			LifeTime;
		float			RadiusRandomness;
		float			BeginRadius;
		float			EndRadius;
		glm::vec4		Color;
		ParticleChunk	ParticleChunk;
		GUID_Lambda		AtlasGUID = 0;
		uint32			TileIndex = 0;
		uint32			AnimationCount = 0;
		uint32			FirstAnimationIndex = 0;
		float			FrictionFactor;
		float			Bounciness;
	};

	struct SParticle
	{
		glm::mat4 Transform;
		glm::vec3 Velocity;
		float CurrentLife;
		glm::vec3 StartVelocity;
		float Padding0;
		glm::vec3 Acceleration;
		uint32 TileIndex;
		glm::vec3 StartPosition;
		bool WasCreated = true;
		glm::vec3 StartAcceleration;
		float BeginRadius;
		float EndRadius;
		float FrictionFactor;
		float Bounciness;
		float Padding1;
	};

	struct SEmitter
	{
		glm::vec4 Color;
		float LifeTime;
		float Radius;
		uint32 AtlasIndex;
		uint32 AnimationCount;
		uint32 FirstAnimationIndex;
		float Gravity;
		bool OneTime;
		uint32 Padding0 = 0;
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

	struct SParticleIndexData
	{
		uint32 EmitterIndex				= 0;
		uint32 ASInstanceIndirectIndex	= 0;
		uint32 Padding0					= 0;
		uint32 Padding1					= 0;
	};

	using EmitterID = uint32;

	class ParticleManager
	{
	public:
		ParticleManager() = default;
		~ParticleManager() = default;

		/*
		*	Destruction of ASBuilder is not ParticleManagers responsibility, but access is needed to update raytraced particles
		*/
		void Init(uint32 maxParticleCapacity, ASBuilder* pASBuilder);
		void Release();

		void Tick(Timestamp deltaTime, uint64 modFrameIndex);

		void UpdateParticleEmitter(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp);

		void OnEmitterEntityRemoved(Entity entity);

		uint32 GetParticleCount() const { return m_AliveIndices.GetSize();  }
		uint32 GetActiveEmitterCount() const { return m_IndirectData.GetSize();  }
		uint32 GetMaxParticleCount() const { return m_MaxParticleCount; }

		bool UpdateBuffers(CommandList* pCommandList);
		bool UpdateResources(RenderGraph* pRendergraph);

	private:
		bool CreateAtlasTextureInstance(GUID_Lambda atlasGUID, uint32 tileSize);

		void UpdateAliveParticles();

		void UpdateEmitterInstanceData(ParticleEmitterInstance& emitterInstance, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp);

		void ReplaceRemovedEmitterWithLast(uint32 removeIndex);

		bool CreateConeParticleEmitter(EmitterID emitterID);
		bool CreateTubeParticleEmitter(EmitterID emitterID);
		bool CopyDataToBuffer(CommandList* pCommandList, void* data, uint32* pOffsets, uint32* pSize, uint32 regionCount, size_t elementSize, Buffer** ppStagingBuffers, Buffer** ppBuffer, FBufferFlags flags, const String& name);

		bool ActivateEmitterInstance(EmitterID emitterID, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp);
		bool DeactivateEmitterInstance(EmitterID emitterID);

		bool AllocateParticleChunk(ParticleChunk& chunk);
		bool FreeParticleChunk(ParticleChunk chunk);
		bool MergeParticleChunk(const ParticleChunk& chunk);

		void CleanBuffers();

	private:
		uint32								m_MaxParticleCount;
		uint64								m_ModFrameIndex;

		bool								m_CreatedDummyBuffer		= false;

		bool								m_DirtyAliveBuffer			= false;
		bool								m_DirtyEmitterIndexBuffer	= false;
		bool								m_DirtyParticleBuffer		= false;
		bool								m_DirtyVertexBuffer			= false;
		bool								m_DirtyIndexBuffer			= false;
		bool								m_DirtyTransformBuffer		= false;
		bool								m_DirtyEmitterBuffer		= false;
		bool								m_DirtyIndirectBuffer		= false;
		bool								m_DirtyAtlasDataBuffer		= false;
		bool								m_DirtyAtlasTexture			= false;

		// Raytracing particle data
		ASBuilder*							m_pASBuilder = nullptr;
		uint32								m_BLASIndex = BLAS_UNINITIALIZED_INDEX;

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

		Buffer*								m_ppParticleIndexDataStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pParticleIndexDataBuffer = nullptr;

		Buffer*								m_ppAliveStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pAliveBuffer = nullptr;

		Buffer*								m_ppParticleStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pParticleBuffer = nullptr;

		Buffer*								m_ppAtlasDataStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*								m_pAtlasDataBuffer = nullptr;

		TArray<DeviceChild*>				m_ResourcesToRemove[BACK_BUFFER_COUNT];

		TArray<uint32>						m_AliveIndices;
		TArray<SParticleIndexData>			m_ParticleIndexData;
		TArray<SParticle>					m_Particles;
		// Emitter specfic data
		TArray<IndirectData>				m_IndirectData;
		TArray<SEmitter>					m_EmitterData;
		TArray<glm::mat4>					m_EmitterTransformData;

		TArray<ParticleChunk>				m_DirtyParticleChunks;
		TArray<ParticleChunk>				m_FreeParticleChunks;
		TArray<SAtlasInfo>					m_AtlasInfoData;

		TSharedRef<Sampler>					m_Sampler = nullptr;
		GUID_Lambda							m_DefaultAtlasTextureGUID;
		THashTable<GUID_Lambda, SAtlasInfo>	m_AtlasResources;
		TArray<Texture*>					m_AtlasTextures;
		TArray<TextureView*>				m_AtlasTextureViews;
		TArray<Sampler*>					m_AtlasSamplers;

		TSet<Entity>								m_RepeatEmitters;
		THashTable<Entity, EmitterID>				m_EntityToEmitterID;
		THashTable<EmitterID, Entity>				m_EmitterIDToEntity;
		TArray<ParticleEmitterInstance>				m_Emitters;

	};
}