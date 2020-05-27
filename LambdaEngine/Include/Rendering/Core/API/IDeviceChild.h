#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IGraphicsDevice;
	
	class IDeviceChild
	{
	public:
		DECL_INTERFACE(IDeviceChild);

		/*
		* Decrements the referencecounter for the object
		*   return - Returns the new referencecount for the object
		*/
		virtual uint64 Release() = 0;

		/*
		* Increases the referencecounter for the object
		*   return - Returns the new referencecount for the object
		*/
		virtual uint64 AddRef() = 0;
		
		/*
		* Sets the internal name of the resource used for debugging applications, such as renderdoc
		* or nsight
		*   pName - A nullterminated string for the resource. The string is copied and can therefore be temporary.
		*/
		virtual void SetName(const char* pName) = 0;
		
		/*
		 * Returns the device that created this object
		 *  return - A pointer to the device that created this object
		 */
		virtual const IGraphicsDevice* GetDevice() const = 0;
	};
}
