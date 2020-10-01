#pragma once
#pragma once

#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"

class MainMenuGUI : public Noesis::Grid
{
public:
	MainMenuGUI(const LambdaEngine::String& xamlFile);
	~MainMenuGUI();

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnRayTracingChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnMeshShadersChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
private:
	void SetRenderStagesSleeping();

private:
	bool	m_RayTracingEnabled		= false;
	bool	m_RayTracingSleeping	= false;
	bool	m_MeshShadersSleeping	= false;

	NS_IMPLEMENT_INLINE_REFLECTION_(MainMenuGUI, Noesis::Grid) 
	
};