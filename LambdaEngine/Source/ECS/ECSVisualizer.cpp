#include "ECS/ECSVisualizer.h"

#include "ECS/ECSCore.h"
#include "ECS/Job.h"

#include <imgui.h>

namespace LambdaEngine
{
	ECSVisualizer::ECSVisualizer(JobScheduler* pJobScheduler)
		: m_pJobScheduler(pJobScheduler)
	{}

	void ECSVisualizer::Render()
	{
		ImGui::Begin("ECS");

		std::array<IDDVector<RegularJob>, PHASE_COUNT> regularJobs = m_pJobScheduler->GetRegularJobs();
		const IDDVector<System*>& systems = ECSCore::GetInstance()->GetSystems();

		ImGui::BeginChild("phases_view");
		ImGui::Columns((int)regularJobs.size());

		for (uint32 phase = 0; phase < regularJobs.size(); phase++)
		{
			const String childName = "phase" + std::to_string(phase);
			ImGui::BeginChild(childName.c_str());
			ImGui::Text("Phase %d", phase);
			ImGui::Separator();

			for (uint32 jobID : regularJobs[phase].GetIDs())
			{
				const System* pSystem = systems.IndexID(jobID);
				ImGui::TextUnformatted(pSystem->GetName().c_str());
			}

			ImGui::EndChild();
			ImGui::NextColumn();
		}

		ImGui::EndChild();
		ImGui::End();
	}
}
