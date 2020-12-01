#pragma once

#include "ECS/ComponentOwner.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	class DeviceChild;

	class InheritanceComponentOwner : public ComponentOwner
	{
	public:
		DECL_SINGLETON_CLASS(InheritanceComponentOwner);

		bool Init();
		bool Release();

		void Tick();

	public:
		FORCEINLINE static InheritanceComponentOwner* GetInstance() { return &s_Instance; }

	private:
		void ParentComponentDestructor(ParentComponent& parentComponent, Entity entity);
		void ChildComponentDestructor(ChildComponent& childComponent, Entity entity);

	private:
		static InheritanceComponentOwner s_Instance;
		TSet<Entity> m_DeletedEntities;
	};
}
