#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"
#include "Containers/TSet.h"
#include "Camera.h"
#include "Time/API/Timestamp.h"
#include "Utilities/HashUtilities.h"

#include "Containers/TStack.h"

namespace LambdaEngine
{
	constexpr const uint32 MAX_NUM_AREA_LIGHTS	= 4;
	constexpr const uint32 NUM_RANDOM_SEEDS		= 8192;

	struct Mesh;

	class GraphicsDevice;
	class IAudioDevice;
	class Buffer;
	class Texture;
	class TextureView;
	class CommandAllocator;
	class CommandList;
	class AccelerationStructure;
	class Fence;
	class DeviceAllocator;
	
	enum EHitMask : uint8
	{
		HIT_MASK_NONE			= 0x00,
		HIT_MASK_GAME_OBJECT	= 0x01,
		HIT_MASK_LIGHT			= 0x02,
		HIT_MASK_ALL			= 0xFF,
	};

	enum EAreaLightType : uint8
	{
		NONE		= 0,
		QUAD		= 1,
	};

	struct SceneDesc
	{
		String	Name				= "Scene";
		bool	RayTracingEnabled	= false;
	};

	struct AreaLightObject
	{
		EAreaLightType	Type;
		GUID_Lambda		Material;
	};

	struct GameObject
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};
	
	struct Instance
	{
		glm::mat4	Transform		= glm::mat4(1.0f);
		glm::mat4	PrevTransform	= glm::mat4(1.0f);
		uint32		MaterialSlot	= 0;
		uint32		Padding0;
		uint32		Padding1;
		uint32		Padding2;
	};

	struct DrawArg
	{
		Buffer* pVertexBuffer		= nullptr;
		uint64	VertexBufferSize	= 0;
		Buffer* pIndexBuffer		= nullptr;
		uint32	IndexCount			= 0;
		Buffer* pInstanceBuffer		= nullptr;
		uint64	InstanceBufferSize	= 0;
		uint32	InstanceCount		= 0;
	};

	struct SBTRecord
	{
		uint64	VertexBufferHandle	= 0;
		uint32	VertexCount			= 0;
		uint64	IndexBufferHandle	= 0;
		uint32	IndexCount			= 0;
		uint32	MaterialSlot		= 0;
	};

	struct DirectionalLight
	{
		glm::vec4	Direction;
		glm::vec4	EmittedRadiance;
	};

	struct AreaLight
	{
		uint32		InstanceIndex;
		uint32		Type;
		uint32		Padding0;
		uint32		Padding1;
	};

	class LAMBDA_API Scene
	{
		struct MeshKey
		{
			GUID_Lambda		MeshGUID;
			bool			IsStatic;
			bool			IsAnimated;
			GUID_Lambda		EntityID;
			mutable size_t	Hash = 0;

			size_t GetHash() const
			{
				if (Hash == 0)
				{
					Hash = std::hash<GUID_Lambda>()(MeshGUID);

					if (IsAnimated) HashCombine<GUID_Lambda>(Hash, EntityID);
				}

				return Hash;
			}

			bool operator==(const MeshKey& other) const
			{
				if (MeshGUID != other.MeshGUID)
					return false;

				if (IsStatic != other.IsStatic)
					return false;

				if (IsAnimated)
				{
					if (!other.IsAnimated || EntityID != other.EntityID)
						return false;
				}
				
				return true;
			}
		};

		struct MeshKeyHasher
		{
			size_t operator()(const MeshKey& key) const
			{
				return key.GetHash();
			}
		};

		struct MeshEntry
		{
			Buffer* pVertexBuffer			= nullptr;
			Buffer* pIndexBuffer			= nullptr;
			uint32	IndexCount				= 0;

			Buffer* pInstanceBuffer			= nullptr;
			Buffer* pInstanceStagingBuffer	= nullptr;
			TArray<Instance> Instances;
		};

		struct InstanceKey
		{
			MeshKey MeshKey;
			uint32	InstanceIndex = 0;
		};

		using MeshAndInstancesMap	= THashTable<MeshKey, MeshEntry, MeshKeyHasher>;
		using MaterialMap			= THashTable<GUID_Lambda, uint32>;

		struct LightSetup
		{
			DirectionalLight	DirectionalLight;
			AreaLight			AreaLights[MAX_NUM_AREA_LIGHTS];
			uint32				AreaLightCount;
		};

		struct PerFrameBuffer
		{
			CameraData Camera;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

	public:
		DECL_UNIQUE_CLASS(Scene);

		Scene();
		~Scene();

		bool Init(const SceneDesc& desc);
		bool Finalize();

		void PrepareRender(CommandList* pGraphicsCommandList, CommandList* pComputeCommandList, uint64 frameIndex, uint64 modFrameIndex);

		void AddGameObject(uint32 entityID, const GameObject& gameObject, const glm::mat4& transform, bool isStatic, bool animated);
		void UpdateTransform(uint32 entityID, const glm::mat4& transform);

		void UpdateCamera(const Camera* pCamera);

		//Todo: Make these const
		FORCEINLINE const AccelerationStructure*	GetTLAS() const				{ return m_pTLAS;}

		FORCEINLINE Buffer*						GetPerFrameBuffer()				{ return m_pPerFrameBuffer; }

		FORCEINLINE Texture**					GetAlbedoMaps()					{ return m_ppSceneAlbedoMaps; }
		FORCEINLINE Texture**					GetNormalMaps()					{ return m_ppSceneNormalMaps; }
		FORCEINLINE Texture**					GetAmbientOcclusionMaps()		{ return m_ppSceneAmbientOcclusionMaps; }
		FORCEINLINE Texture**					GetMetallicMaps()				{ return m_ppSceneMetallicMaps; }
		FORCEINLINE Texture**					GetRoughnessMaps()				{ return m_ppSceneRoughnessMaps; }

		FORCEINLINE TextureView**				GetAlbedoMapViews()				{ return m_ppSceneAlbedoMapViews; }
		FORCEINLINE TextureView**				GetNormalMapViews()				{ return m_ppSceneNormalMapViews; }
		FORCEINLINE TextureView**				GetAmbientOcclusionMapViews()	{ return m_ppSceneAmbientOcclusionMapViews; }
		FORCEINLINE TextureView**				GetMetallicMapViews()			{ return m_ppSceneMetallicMapViews; }
		FORCEINLINE TextureView**				GetRoughnessMapViews()			{ return m_ppSceneRoughnessMapViews; }

		FORCEINLINE Buffer*						GetMaterialProperties()			{ return m_pMaterialParametersBuffer; }

		FORCEINLINE bool						IsRayTracingEnabled()			{ return m_RayTracingEnabled; }

		void GetDrawArgs(TArray<DrawArg>& drawArgs, uint32 key) const;

	private:
		void UpdateInstanceBuffers(CommandList* pCommandList, uint64 modFrameIndex);
		void UpdatePerFrameBuffer(CommandList* pCommandList);
		void UpdateMaterialPropertiesBuffer(CommandList* pCommandList, uint64 modFrameIndex);

		void BuildTLAS(CommandList* pBuildCommandList, bool update);

	private:
		MeshAndInstancesMap	m_MeshAndInstancesMap;
		MaterialMap			m_MaterialMap;

		Texture*			m_ppSceneAlbedoMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneNormalMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneAmbientOcclusionMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneRoughnessMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppSceneMetallicMaps[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneAlbedoMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneNormalMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneAmbientOcclusionMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneRoughnessMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppSceneMetallicMapViews[MAX_UNIQUE_MATERIALS];
		MaterialProperties	m_pSceneMaterialProperties[MAX_UNIQUE_MATERIALS];
		Buffer*				m_pMaterialParametersStagingBuffer		= nullptr;
		Buffer*				m_pMaterialParametersBuffer				= nullptr;
		TStack<uint32>		m_FreeMaterialSlots;

		TSet<MeshEntry*>	m_DirtyInstanceBuffers;

		TArray<Buffer*>		m_BuffersToRemove[BACK_BUFFER_COUNT];

		THashTable<GUID_Lambda, InstanceKey> m_EntityIDsToInstanceKey;

		PerFrameBuffer	m_PerFrameData;
		Buffer*			m_pPerFrameStagingBuffer	= nullptr;
		Buffer*			m_pPerFrameBuffer			= nullptr;

		bool							m_RayTracingEnabled		= false;
		AccelerationStructure*			m_pTLAS					= nullptr;
		TArray<AccelerationStructure*>	m_BLASs;
	};
}
