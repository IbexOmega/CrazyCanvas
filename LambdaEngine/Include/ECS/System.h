#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/RegularWorker.h"

#include "Time/API/Timestamp.h"

#include <functional>
#include <typeindex>

namespace LambdaEngine
{
	struct SystemRegistration
	{
		EntitySubscriberRegistration SubscriberRegistration;
		uint32 Phase = 0;
		uint32 TickFrequency = 0;
	};

	class ComponentHandler;

	// A system processes components in the Tick function
	class LAMBDA_API System : EntitySubscriber, RegularWorker
	{
	public:
		virtual ~System();

		virtual void Tick(Timestamp deltaTime) = 0;

		const String& GetName() const { return m_SystemName; }

	protected:
		void RegisterSystem(const String& systemName, SystemRegistration& systemRegistration);

	private:
		String m_SystemName;
	};
}
