#pragma once

#include "ECS/System.h"

#undef realloc
#undef free
#include <PxPhysicsAPI.h>
#include <foundation/PxErrorCallback.h>

namespace LambdaEngine
{
	using namespace physx;

	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		static PhysicsSystem s_Instance;

	private:
		PxDefaultAllocator		m_Allocator;
		PxDefaultErrorCallback	m_ErrorCallback;

		PxFoundation*	m_pFoundation;
		PxPhysics*		m_pPhysics;

		PxDefaultCpuDispatcher*	m_pDispatcher;
		PxScene*				m_pScene;
	};
}
