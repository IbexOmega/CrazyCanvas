#pragma once

#include "Physics/PhysX/PhysX.h"

/*	This filter shader controls which collisions are registered by PhysX. It is a copy-paste from the PhysX SDK's
	sample called "SampleSubmarine". A thorough explanation can be found here:
	https://gameworksdocs.nvidia.com/PhysX/4.0/documentation/PhysXGuide/Manual/RigidBodyCollision.html#collision-filtering */
physx::PxFilterFlags FilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* pConstantBlock, physx::PxU32 constantBlockSize)
{
	using namespace physx;

	UNREFERENCED_VARIABLE(pConstantBlock);
	UNREFERENCED_VARIABLE(constantBlockSize);

	// Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
	{
		if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
		{
			// Call onTrigger when PxScene::fetchResults is called
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		}
		else
		{
			// Call onContact when PxScene::fetchResults is called
			pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND |
						PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}

		return PxFilterFlag::eDEFAULT;
	}

	return PxFilterFlag::eSUPPRESS;
}
