#pragma once
#include "LambdaEngine.h"

#include "Containers/String.h"

#include "Core/RefCountedObject.h"

namespace LambdaEngine
{
	class GraphicsDevice;
	
	class DeviceChild : public RefCountedObject
	{
	public:
		DECL_ABSTRACT_CLASS(DeviceChild);

		/*
		* Sets the internal name of the resource used for debugging applications, such as renderdoc
		* or nsight
		*   name - A string that is the new name for the resource.
		*/
		virtual void SetName(const String& name) = 0;
		
		/*
		 * Returns the device that created this object
		 *  return - A pointer to the device that created this object
		 */
		virtual const GraphicsDevice* GetDevice() const = 0;
	};
}
