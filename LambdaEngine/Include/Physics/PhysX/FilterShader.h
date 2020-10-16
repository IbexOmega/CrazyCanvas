#pragma once

#undef realloc
#undef free
#include <PxPhysicsAPI.h>

/*	This filter shader controls which collisions are registered by PhysX. It is a copy-paste from the PhysX SDK's
	sample called "SampleSubmarine". A thorough explanation can be found here:
	https://gameworksdocs.nvidia.com/PhysX/4.0/documentation/PhysXGuide/Manual/RigidBodyCollision.html#collision-filtering */
physx::PxFilterFlags FilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* pConstantBlock, physx::PxU32 constantBlockSize)
{
	using namespace physx;

	// Let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}

	// Generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	// Trigger the contact callback for pairs (A,B) where the filtermask of A contains the ID of B and vice versa
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

	return PxFilterFlag::eDEFAULT;
}
