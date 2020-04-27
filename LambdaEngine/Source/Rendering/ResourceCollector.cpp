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

	void ResourceCollector::DisposeResource(IDeviceChild* pObject)
	{
		VALIDATE(pObject != nullptr);
		m_Objects.push_back(pObject);
	}

	void ResourceCollector::Reset()
	{
		for (IDeviceChild* pObject : m_Objects)
		{
			pObject->Release();
		}

		m_Objects.clear();
	}
}