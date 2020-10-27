#include "ECS/ECSVisualizer.h"

#include "ECS/ECSCore.h"
#include "ECS/Job.h"

#include "Game/GameConsole.h"

#include "Rendering/ImGuiRenderer.h"

#include <imgui.h>

namespace LambdaEngine
{
	ECSVisualizer::ECSVisualizer(JobScheduler* pJobScheduler)
		: m_pJobScheduler(pJobScheduler)
	{
#ifdef LAMBDA_DEVELOPMENT
		ConsoleCommand cmd;
		cmd.Init("show_ecs", false);
		cmd.AddArg(Arg::EType::BOOL);
		cmd.AddDescription("Activate/Deactivate ECS visualization window.\n\t'show_ecs true'");
		GameConsole::Get().BindCommand(cmd, [this](GameConsole::CallbackInput& input) {
			m_Enabled = input.Arguments.GetFront().Value.Boolean;
		});
#endif
	}

	void ECSVisualizer::Render()
	{
		if (m_Enabled)
		{
			ImGuiRenderer::Get().DrawUI([this]()
			{
				ImGui::Begin("ECS");

				const std::array<IDDVector<RegularJob>, PHASE_COUNT>& regularJobs = m_pJobScheduler->GetRegularJobs();
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
			});
		}
	}
}
