#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	enum class EAccelerationStructureType
	{
		ACCELERATION_STRUCTURE_TYPE_NONE	= 0,
		ACCELERATION_STRUCTURE_TYPE_TOP		= 1,
		ACCELERATION_STRUCTURE_TYPE_BOTTOM	= 2,
	};

	struct AccelerationStructureDesc
	{
		String						DebugName			= "";
		EAccelerationStructureType	Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_NONE;
		uint32						Flags				= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
		uint32						InstanceCount		= 8;
		uint32						MaxTriangleCount	= 0;
		uint32						MaxVertexCount		= 0;
		bool						AllowsTransform		= false;
	};

	class AccelerationStructure : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(AccelerationStructure);

		/*
		* Returns this resource's address on the device
		*	return -	Returns a valid 64-bit address on success, otherwise zero. Returns zero on systems that
		*				does not support deviceaddresses.
		*/
		virtual uint64 GetDeviceAdress() const
		{
			return 0ULL;
		}

		/*
		* Returns the API-specific handle to the underlaying TopLevelAccelerationStructure-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;
		
		FORCEINLINE const AccelerationStructureDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		AccelerationStructureDesc m_Desc;
	};
}
