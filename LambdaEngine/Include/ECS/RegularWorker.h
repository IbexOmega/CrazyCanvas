#pragma once

#include "Containers/THashTable.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/Job.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	struct RegularWorkInfo
	{
		std::function<void(Timestamp deltaTime)> TickFunction;
		EntitySubscriberRegistration EntitySubscriberRegistration;
		uint32 Phase;
		float32 TickPeriod;
	};

	// RegularWorker schedules a regular job and deregisters it upon destruction
	class RegularWorker
	{
	public:
		RegularWorker() = default;
		~RegularWorker();

		void Tick();

		void ScheduleRegularWork(const RegularWorkInfo& regularWorkInfo);

	protected:
		uint32 GetJobID() const { return m_JobID; }

	protected:
		// GetUniqueComponentAccesses serializes all unique component accesses in an entity subscriber registration
		static TArray<ComponentAccess> GetUniqueComponentAccesses(const EntitySubscriberRegistration& subscriberRegistration);

	private:
		static void MapComponentAccesses(const TArray<ComponentAccess>& componentAccesses, THashTable<const ComponentType*, ComponentPermissions>& uniqueRegs);

	private:
		uint32 m_Phase = UINT32_MAX;
		uint32 m_JobID = UINT32_MAX;

		float32 m_TickPeriod = -1.0f;

		std::function<void(Timestamp deltaTime)> m_TickFunction = nullptr;
	};
}
