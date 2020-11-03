#include "EventHandlers/AudioEffectHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Rendering/PaintMaskRenderer.h"

AudioEffectHandler::~AudioEffectHandler()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ProjectileHitEvent, AudioEffectHandler>(this, &AudioEffectHandler::OnProjectileHit);
}

void AudioEffectHandler::Init()
{
	using namespace LambdaEngine;
	GUID_Lambda projectileHitID = ResourceManager::LoadSoundEffect3DFromFile("9_mm_gunshot-mike-koenig-123.wav");
	m_pProjectileHitSound = ResourceManager::GetSoundEffect3D(projectileHitID);

	EventQueue::RegisterEventHandler<ProjectileHitEvent, AudioEffectHandler>(this, &AudioEffectHandler::OnProjectileHit);
}

bool AudioEffectHandler::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	UNREFERENCED_VARIABLE(projectileHitEvent);
	using namespace LambdaEngine;

	//m_pProjectileHitSound->PlayOnceAt(projectileHitEvent.CollisionInfo0.Position);
	return true;
}
