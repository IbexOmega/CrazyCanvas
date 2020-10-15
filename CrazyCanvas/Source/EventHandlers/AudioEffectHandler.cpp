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
	GUID_Lambda projectileHitID = ResourceManager::LoadSoundEffectFromFile("Fart.wav");
	m_pProjectileHitSound = ResourceManager::GetSoundEffect(projectileHitID);

	EventQueue::RegisterEventHandler<ProjectileHitEvent, AudioEffectHandler>(this, &AudioEffectHandler::OnProjectileHit);
}

bool AudioEffectHandler::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;
	m_pProjectileHitSound->PlayOnceAt(projectileHitEvent.CollisionInfo.Position);
	return true;
}
