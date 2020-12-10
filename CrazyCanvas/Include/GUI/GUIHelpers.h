#pragma once

#include "Containers/String.h"

#include "NsGui/Panel.h"
#include "NsGui/UICollection.h"
#include "NsGui/ColumnDefinition.h"
#include "NsGui/RowDefinition.h"
#include "NsGui/Grid.h"
#include "NsGui/FrameworkElement.h"
#include "NsGui/Label.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Engine/EngineConfig.h"
#include "Rendering/RenderAPI.h"

#include "Rendering/RenderGraph.h"

FORCEINLINE void AddColumnDefinition(Noesis::ColumnDefinitionCollection* pColumnCollection, float width, Noesis::GridUnitType unit)
{
	using namespace Noesis;

	GridLength gl = GridLength(width, unit);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	pColumnCollection->Add(col);
}

FORCEINLINE void AddRowDefinition(Noesis::RowDefinitionCollection* pRowCollection, float height, Noesis::GridUnitType unit)
{
	using namespace Noesis;

	GridLength gl = GridLength(height, unit);
	Ptr<RowDefinition> row = *new RowDefinition();
	row->SetHeight(gl);
	pRowCollection->Add(row);
}

FORCEINLINE void DisablePlaySessionsRenderstages()
{
	using namespace LambdaEngine;
	RenderSystem& rs = RenderSystem::GetInstance();

	rs.SetRenderStageSleeping("SKYBOX_PASS",						true);
	rs.SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",				true);
	rs.SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT",	true);
	rs.SetRenderStageSleeping("DIRL_SHADOWMAP",						true);
	rs.SetRenderStageSleeping("FXAA",								true);
	rs.SetRenderStageSleeping("RENDER_STAGE_LIGHT",					true);
	rs.SetRenderStageSleeping("RENDER_STAGE_PARTICLE_RENDER",		true);
	rs.SetRenderStageSleeping("PARTICLE_COMBINE_PASS",				true);
	rs.SetRenderStageSleeping("PLAYER_PASS",						true);
	rs.SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON",	true);
	rs.SetRenderStageSleeping("SHADING_PASS",						true);
	rs.SetRenderStageSleeping(BLIT_STAGE,							true);
	rs.SetRenderStageSleeping(REFLECTIONS_DENOISE_PASS,				true);
	rs.SetRenderStageSleeping("RENDER_STAGE_PROJECTILES",			true);

	rs.SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	// Check if raytracing is enabled/supported
	GraphicsDeviceFeatureDesc deviceFeatures;
	RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
	bool rayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);

	if (rayTracingEnabled)
		rs.SetRenderStageSleeping("RAY_TRACING",					true);
}

FORCEINLINE void EnablePlaySessionsRenderstages()
{
	using namespace LambdaEngine;
	RenderSystem& rs = RenderSystem::GetInstance();

	// Put unecessary renderstages to sleep in main menu
	rs.SetRenderStageSleeping("SKYBOX_PASS",						false);
	rs.SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",				false);
	rs.SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT",	false);
	rs.SetRenderStageSleeping("DIRL_SHADOWMAP",						false);
	rs.SetRenderStageSleeping("FXAA",								false);
	rs.SetRenderStageSleeping("RENDER_STAGE_LIGHT",					false);
	rs.SetRenderStageSleeping("RENDER_STAGE_PARTICLE_RENDER",		false);
	rs.SetRenderStageSleeping("PARTICLE_COMBINE_PASS",				false);
	rs.SetRenderStageSleeping("PLAYER_PASS",						false);
	rs.SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON",	false);
	rs.SetRenderStageSleeping("SHADING_PASS",						false);
	rs.SetRenderStageSleeping(REFLECTIONS_DENOISE_PASS,				false);
	rs.SetRenderStageSleeping(BLIT_STAGE,							false);
	rs.SetRenderStageSleeping("RENDER_STAGE_PROJECTILES",			false);

	rs.SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",			false);

	// Check if raytracing is enabled/supported
	GraphicsDeviceFeatureDesc deviceFeatures;
	RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
	bool rayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);

	if (rayTracingEnabled)
		rs.SetRenderStageSleeping("RAY_TRACING",					false);
}

FORCEINLINE void ChangeGlossySettings(bool glossyEnabled, int32 spp)
{
	using namespace LambdaEngine;
	RenderSystem& rs = RenderSystem::GetInstance();
	RenderGraph* pRenderGraph = rs.GetRenderGraph();

	struct
	{
		int32 GlossyEnabled;
		int32 SPP;
	} rayTracingPushConstant;

	rayTracingPushConstant.GlossyEnabled	= int32(glossyEnabled);
	rayTracingPushConstant.SPP				= spp;

	PushConstantsUpdate pushContantUpdate = {};
	pushContantUpdate.pData				= &rayTracingPushConstant;
	pushContantUpdate.DataSize			= sizeof(rayTracingPushConstant);

	{
		pushContantUpdate.RenderStageName = "RAY_TRACING";
		pRenderGraph->UpdatePushConstants(&pushContantUpdate);
	}

	{
		pushContantUpdate.RenderStageName = "SHADING_PASS";
		pRenderGraph->UpdatePushConstants(&pushContantUpdate);
	}

	{
		pushContantUpdate.RenderStageName = "REFLECTIONS_DENOISE_PASS";
		pRenderGraph->UpdatePushConstants(&pushContantUpdate);
	}
}