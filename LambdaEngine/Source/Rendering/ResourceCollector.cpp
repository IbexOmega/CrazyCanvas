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
		m_Objects.emplace_back(pObject);
	}

	void ResourceCollector::Reset()
	{
		for (Ref<DeviceChild>& object : m_Objects)
		{
			object.Reset();
		}

		m_Objects.clear();
	}
}