#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	enum class EAccelerationStructureType
	{
		ACCELERATION_STRUCTURE_NONE		= 0,
		ACCELERATION_STRUCTURE_TOP		= 1,
		ACCELERATION_STRUCTURE_BOTTOM	= 2,
	};

	struct AccelerationStructureDesc
	{
		const char*					pName				= "";
		EAccelerationStructureType	Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_NONE;
		uint32						Flags				= 0;
		uint32						InstanceCount		= 8;
		uint32						MaxTriangleCount	= 0;
		uint32						MaxVertexCount		= 0;
	};

	class IAccelerationStructure : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IAccelerationStructure);

		/*
		* Getter for the minimum memory size requirement of a scratch buffer that will be used to build/refit this TopLevelAS
		*
		* return - The minimum size in bytes
		*/
		virtual uint64 GetScratchMemorySizeRequirement() const = 0;

		/*
		* Returns the alignment needed for the buffer when using a buffer offset
		*
		* return - Returns the needed alignement on success otherwise zero
		*/
		virtual uint64 GetScratchMemoryAlignmentRequirement() const = 0;

		/*
		* Returns this resource's address on the device
		*
		* return -  Returns a valid 64-bit address on success, otherwise zero. Returns zero on systems that
		*           does not support deviceaddresses.
		*/
		virtual uint64 GetDeviceAdress() const = 0;

		/*
		* Returns the API-specific handle to the underlaying TopLevelAccelerationStructure-resource
		*
		* return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		virtual AccelerationStructureDesc GetDesc() const = 0;
	};
}