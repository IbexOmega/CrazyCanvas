#include "States/MainMenuState.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"

MainMenuState::~MainMenuState()
{
	int32 ref = m_MainMenuGUI->GetNumReferences();

	m_MainMenuGUI.Reset();
	m_View.Reset();
}

void MainMenuState::Init()
{
	using namespace LambdaEngine;

	//String prefix = m_RayTracingEnabled ? "RT_" : "";
	//String postfix = m_MeshShadersEnabled ? "_MESH" : "";
	//String renderGraphName = EngineConfig::GetStringProperty("RenderGraphName");
	//size_t pos = renderGraphName.find_first_of(".lrg");
	//if (pos != String::npos)
	//{
	//	renderGraphName.insert(pos, postfix);
	//}
	//else
	//{
	//	renderGraphName += postfix + ".lrg";
	//}

	RenderGraphStructureDesc renderGraphStructure = {};

	m_MainMenuGUI = *new MainMenuGUI("MainMenu.xaml");
	m_View = Noesis::GUI::CreateView(m_MainMenuGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}

void MainMenuState::Tick(LambdaEngine::Timestamp delta)
{
	
}
