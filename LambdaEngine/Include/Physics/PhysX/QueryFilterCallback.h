#pragma once
#include "Physics/PhysX/PhysX.h"

namespace LambdaEngine
{
	class QueryFilterCallback : public physx::PxQueryFilterCallback
	{
	public:
		DECL_UNIQUE_CLASS(QueryFilterCallback);

		QueryFilterCallback() = default;
		~QueryFilterCallback() = default;

		inline virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* pShape, const physx::PxRigidActor* pActor, physx::PxHitFlags& queryFlags) override final
		{
			UNREFERENCED_VARIABLE(pActor);
			UNREFERENCED_VARIABLE(queryFlags);

			using namespace physx;

			// We only need to check word2 as it has already passed dataFilter, we also use SimulationFilterData for this
			if (filterData.word2 != pShape->getSimulationFilterData().word2)
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
	};
}