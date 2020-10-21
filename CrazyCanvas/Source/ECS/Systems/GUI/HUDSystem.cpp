#include "ECS/Systems/GUI/HUDSystem.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"


using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();
}

void HUDSystem::Init()
{

	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_WeaponEntities,
			.ComponentAccesses =
			{
				{R, WeaponComponent::Type()}
			}
		},
	};
	systemReg.SubscriberRegistration.AdditionalAccesses =
	{
		{R, PlayerLocalComponent::Type()}
	};

	RegisterSystem(systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	m_HUDGUI = *new HUDGUI("HUD.xaml");
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	GUIApplication::SetView(m_View);

}

void HUDSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
}

void HUDSystem::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

	for (Entity weaponEntity : m_WeaponEntities)
	{
		const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(weaponEntity);
		Entity playerEntity = weaponComponent.WeaponOwner;

		if (pPlayerLocalComponents->HasComponent(playerEntity) && m_HUDGUI)
		{
			m_HUDGUI->UpdateAmmo(weaponComponent.CurrentAmmunition, weaponComponent.AmmoCapacity);
		}
	}
}