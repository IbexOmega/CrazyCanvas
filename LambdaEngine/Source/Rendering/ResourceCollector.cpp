#include "Rendering/ResourceCollector.h"

namespace LambdaEngine
{
	ResourceCollector::ResourceCollector()
		: m_Objects()
	{
	}

	ResourceCollector::~ResourceCollector()
	{
		Reset();
	}

	void ResourceCollector::DisposeResource(DeviceChild* pObject)
	{
		VALIDATE(pObject != nullptr);
		m_Objects.EmplaceBack(pObject);
	}

	void ResourceCollector::Reset()
	{
		for (TSharedRef<DeviceChild>& object : m_Objects)
		{
			object.Reset();
		}

		m_Objects.Clear();
	}
}