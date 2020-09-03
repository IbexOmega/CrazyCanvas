#pragma once
#include "Core/TSharedRef.h"

#include "Core/API/DeviceChild.h"

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
		* Handles destruction of a DeviceChild* object. Can be called instead of SAFERELEASE macro or DeviceChild::Release.
		* Call to ResourceCollector::Reset to destroy all collected resources.
		*	pObject - Object to remove
		*/
		void DisposeResource(DeviceChild* pObject);
		
		/*
		* Destroys all objects collected with ResourceCollector::DisposeResource
		*/
		void Reset();

	private:
		TArray<TSharedRef<DeviceChild>> m_Objects;
	};
}