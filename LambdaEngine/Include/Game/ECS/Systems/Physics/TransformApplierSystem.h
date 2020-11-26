#pragma once

#include "ECS/System.h"

namespace LambdaEngine
{
	/*	TransformApplierSystem runs after all transforms logic has been performed. Reads transform data eg. to update
		update view and camera matrices. */
	class TransformApplierSystem : public System
	{
	public:
		TransformApplierSystem() = default;
		~TransformApplierSystem() = default;

		void Init();

		void Tick(Timestamp deltaTime) override final;

		static TransformApplierSystem* GetInstance() { return &s_Instance; }

	private:
		static TransformApplierSystem s_Instance;

	private:
		IDVector m_MatrixEntities;
		IDVector m_VelocityEntities;
		uint64 m_Tick = 0;
	};
}
