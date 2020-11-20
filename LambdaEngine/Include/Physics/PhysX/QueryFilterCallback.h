#pragma once
#include "Physics/PhysX/PhysX.h"

namespace LambdaEngine
{
	class QueryFilterCallback : public physx::PxQueryFilterCallback
	{
	public:
		DECL_SINGLETON_CLASS(QueryFilterCallback);

		inline virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override final
		{
			UNREFERENCED_VARIABLE(actor);
			UNREFERENCED_VARIABLE(queryFlags);

			using namespace physx;

			// We only need to check word2 as it has already passed dataFilter, we also use SimulationFilterData for this
			if (filterData.word2 != shape->getSimulationFilterData().word2)
			{
				return PxQueryHitType::eTOUCH;
			}

			return PxQueryHitType::eNONE;
		}

		inline virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override final
		{
			UNREFERENCED_VARIABLE(filterData);
			UNREFERENCED_VARIABLE(hit);

			return physx::PxQueryHitType::eTOUCH;
		}

		FORCEINLINE static QueryFilterCallback* GetInstance() 
		{
			static QueryFilterCallback callbackInstance;
			return &callbackInstance;
		}
		
	private:
		static QueryFilterCallback s_Instance;
	};
}