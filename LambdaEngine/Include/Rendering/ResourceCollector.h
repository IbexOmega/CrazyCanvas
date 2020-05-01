#pragma once
#include "Core/API/IDeviceChild.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class ResourceCollector
	{
	public:
		DECL_UNIQUE_CLASS(ResourceCollector);

		ResourceCollector();
		~ResourceCollector();

		/*
		* Handles destruction of a IDeviceChild* object. Can be called instead of SAFERELEASE macro or IDeviceChild::Release.
		* Call to ResourceCollector::Reset to destroy all collected resources.
		*
		* pObject - Object to remove
		*/
		void DisposeResource(IDeviceChild* pObject);
		
		/*
		* Destroys all objects collected with ResourceCollector::DisposeResource
		*/
		void Reset();

	private:
		TArray<IDeviceChild*> m_Objects;
	};
}