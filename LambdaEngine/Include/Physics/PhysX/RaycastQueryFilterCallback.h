#pragma once
#include "Physics/PhysX/PhysX.h"

namespace LambdaEngine
{
	// See RaycastFilterData for explanation of the words
	class RaycastQueryFilterCallback : public physx::PxQueryFilterCallback
	{
	public:
		DECL_UNIQUE_CLASS(RaycastQueryFilterCallback);

		RaycastQueryFilterCallback() = default;
		~RaycastQueryFilterCallback() = default;

		inline virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* pShape, const physx::PxRigidActor* pActor, physx::PxHitFlags& queryFlags) override final
		{
			UNREFERENCED_VARIABLE(pActor);
			UNREFERENCED_VARIABLE(queryFlags);

			using namespace physx;

			const PxFilterData shapeFilterData = pShape->getQueryFilterData();
			if (((filterData.word0 & shapeFilterData.word0) != 0) && ((filterData.word1 & shapeFilterData.word0) == 0) && filterData.word2 != shapeFilterData.word2)
			{
				return PxQueryHitType::eBLOCK;
			}

			return PxQueryHitType::eNONE;
		}

		inline virtual physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) override final
		{
			UNREFERENCED_VARIABLE(filterData);
			UNREFERENCED_VARIABLE(hit);

			return physx::PxQueryHitType::eBLOCK;
		}
	};

	/*	Identical to the callback above, but each hit is touching as opposed to blocking.
		I.e. a ray can hit multiple objects. */
	class RaycastTouchingQueryFilterCallback : public physx::PxQueryFilterCallback
	{
	public:
		DECL_UNIQUE_CLASS(RaycastTouchingQueryFilterCallback);

		RaycastTouchingQueryFilterCallback() = default;
		~RaycastTouchingQueryFilterCallback() = default;

		inline virtual physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* pShape, const physx::PxRigidActor* pActor, physx::PxHitFlags& queryFlags) override final
		{
			UNREFERENCED_VARIABLE(pActor);
			UNREFERENCED_VARIABLE(queryFlags);

			using namespace physx;

			const PxFilterData shapeFilterData = pShape->getQueryFilterData();
			if (((filterData.word0 & shapeFilterData.word0) != 0) && ((filterData.word1 & shapeFilterData.word0) == 0) && filterData.word2 != shapeFilterData.word2)
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
