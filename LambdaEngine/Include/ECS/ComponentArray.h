#pragma once

#include "Containers/THashTable.h"
#include "Defines.h"
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include <type_traits>

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

namespace LambdaEngine
{
	class ComponentStorage;

	#pragma pack(push, 1)
		struct ComponentSerializationHeader
		{
			uint32 TotalSerializationSize; // Size of header + component data
			uint32 TypeHash;
		};
	#pragma pack(pop)

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;

		virtual void UnsetComponentOwner() = 0;

		virtual const TArray<uint32>& GetIDs() const = 0;

		virtual uint32 SerializeComponent(Entity entity, uint8* pBuffer, uint32 bufferSize) const = 0;
		// DeserializeComponent adds a component if it does not already exist, otherwise the existing component is updated
		virtual bool DeserializeComponent(Entity entity, const uint8* pBuffer, uint32 serializationSize, bool& entityHadComponent) = 0;

		virtual bool HasComponent(Entity entity) const = 0;
		virtual void ResetDirtyFlags() = 0;

		virtual void* GetRawData(Entity entity) = 0;

	protected:
		// Systems or other external users should not be able to perform immediate deletions
		friend ComponentStorage;
		virtual void Remove(Entity entity) = 0;
	};

	template<typename Comp>
	class LAMBDA_API ComponentArray : public IComponentArray
	{
	public:
		ComponentArray() = default;
		~ComponentArray() override final;

		void SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership) { m_ComponentOwnership = componentOwnership; }
		void UnsetComponentOwner() override final { m_ComponentOwnership = {}; }

		Comp& Insert(Entity entity, const Comp& comp);

		Comp& GetData(Entity entity);
		const Comp& GetConstData(Entity entity) const;

		void* GetRawData(Entity entity) override final;

		const TArray<uint32>& GetIDs() const override final { return m_IDs; }

		uint32 SerializeComponent(Entity entity, uint8* pBuffer, uint32 bufferSize) const override final { return SerializeComponent(GetConstData(entity), pBuffer, bufferSize); }
		uint32 SerializeComponent(const Comp& component, uint8* pBuffer, uint32 bufferSize) const;
		bool DeserializeComponent(Entity entity, const uint8* pBuffer, uint32 serializationSize, bool& entityHadComponent);

		bool HasComponent(Entity entity) const override final { return m_EntityToIndex.find(entity) != m_EntityToIndex.end(); }
		void ResetDirtyFlags() override final;

	protected:
		void Remove(Entity entity) override final;

	private:
		TArray<Comp> m_Data;
		TArray<uint32> m_IDs;
		THashTable<Entity, uint32> m_EntityToIndex;

		ComponentOwnership<Comp> m_ComponentOwnership;
	};

	template<typename Comp>
	inline ComponentArray<Comp>::~ComponentArray()
	{
		if (m_ComponentOwnership.Destructor)
		{
			for (Comp& component : m_Data)
			{
				m_ComponentOwnership.Destructor(component);
			}
		}
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::Insert(Entity entity, const Comp& comp)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr == m_EntityToIndex.end(), "Trying to add a component that already exists!");

		// Get new index and add the component to that position.
		uint32 newIndex = m_Data.GetSize();
		m_EntityToIndex[entity] = newIndex;
		m_IDs.PushBack(entity);
		return m_Data.PushBack(comp);
	}

	template<typename Comp>
	inline void* ComponentArray<Comp>::GetRawData(Entity entity)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to get a component that does not exist!");
		return &m_Data[indexItr->second];
	}

	template<typename Comp>
	inline Comp& ComponentArray<Comp>::GetData(Entity entity)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to get a component that does not exist!");

		Comp& component = m_Data[indexItr->second];

		if constexpr (Comp::HasDirtyFlag())
		{
			component.Dirty = true;
		}

		return component;
	}

	template<typename Comp>
	inline const Comp& ComponentArray<Comp>::GetConstData(Entity entity) const
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to get a component that does not exist!");

		return m_Data[indexItr->second];
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::Remove(Entity entity)
	{
		auto indexItr = m_EntityToIndex.find(entity);
		VALIDATE_MSG(indexItr != m_EntityToIndex.end(), "Trying to remove a component that does not exist!");

		uint32 currentIndex = indexItr->second;

		if (m_ComponentOwnership.Destructor)
		{
			m_ComponentOwnership.Destructor(m_Data[currentIndex]);
		}

		// Swap the removed component with the last component.
		m_Data[currentIndex] = m_Data.GetBack();
		m_IDs[currentIndex] = m_IDs.GetBack();

		// Update entity-index maps.
		m_EntityToIndex[m_IDs.GetBack()] = currentIndex;

		m_Data.PopBack();
		m_IDs.PopBack();

		// Remove the deleted component's entry.
		m_EntityToIndex.erase(indexItr);
	}

	template<typename Comp>
	inline uint32 ComponentArray<Comp>::SerializeComponent(const Comp& component, uint8* pBuffer, uint32 bufferSize) const
	{
		/*	ComponentSerializationHeader is written to the beginning of the buffer. This is done last, when the size of
			the serialization is known. */
		uint8* pHeaderPosition = pBuffer;
		constexpr const uint32 headerSize = sizeof(ComponentSerializationHeader);
		const bool hasRoomForHeader = bufferSize >= headerSize;
		if (hasRoomForHeader)
		{
			pBuffer		+= headerSize;
			bufferSize	-= headerSize;
		}

		uint32 requiredTotalSize = headerSize;

		// Use a component owner's serialize function, or memcpy the component directly
		if (m_ComponentOwnership.Serialize)
		{
			requiredTotalSize += m_ComponentOwnership.Serialize(component, pBuffer, bufferSize);
		}
		else if constexpr (std::is_trivially_copyable<Comp>::value)
		{
			// The if-statements have to be nested to avoid the compiler warning: 'use constexpr on if-statement'
			if (bufferSize >= sizeof(Comp))
			{
				constexpr const uint32 componentSize = sizeof(Comp);
				memcpy(pBuffer, &component, componentSize);
				requiredTotalSize += componentSize;
			}
		}

		// Finalize the serialization by writing the header
		if (hasRoomForHeader)
		{
			const ComponentSerializationHeader header =
			{
				.TotalSerializationSize	= requiredTotalSize,
				.TypeHash				= Comp::Type()->GetHash()
			};

			memcpy(pHeaderPosition, &header, headerSize);
		}

		return requiredTotalSize;
	}

	template<typename Comp>
	inline bool ComponentArray<Comp>::DeserializeComponent(Entity entity, const uint8* pBuffer, uint32 serializationSize, bool& entityHadComponent)
	{
		Comp component = {};
		Comp* pComponent = &component;
		entityHadComponent = false;
		if (HasComponent(entity))
		{
			entityHadComponent = true;
			pComponent = &GetData(entity);
		}

		if (m_ComponentOwnership.Deserialize)
		{
			return m_ComponentOwnership.Deserialize(*pComponent, serializationSize, pBuffer);
		}

		memcpy(pComponent, pBuffer, serializationSize);
		if (!entityHadComponent)
		{
			Insert(entity, *pComponent);
		}

		return true;
	}

	template<typename Comp>
	inline void ComponentArray<Comp>::ResetDirtyFlags()
	{
		if constexpr (Comp::HasDirtyFlag())
		{
			for (Comp& component : m_Data)
			{
				component.Dirty = false;
			}
		}
	}
}
