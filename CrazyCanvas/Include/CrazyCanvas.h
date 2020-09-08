#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"
#include "CameraTrack.h"

#include "Containers/TArray.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct GameObject;

	class RenderGraph;
	class Renderer;
	class ResourceManager;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class Scene;
	class Camera;
	class Sampler;

	class RenderGraphEditor;
}

class CrazyCanvas : public LambdaEngine::Game, public LambdaEngine::EventHandler
{
	struct InstanceIndexAndTransform
	{
		uint32		InstanceIndex;
		glm::vec3	Position;
		glm::vec4	Rotation;
		glm::vec3	Scale;
	};

public:
	CrazyCanvas();
	~CrazyCanvas();

	void InitTestAudio();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	void Render(LambdaEngine::Timestamp delta);

private:
	bool InitRendererForDeferred();

private:
	CameraTrack m_CameraTrack;

	uint32									m_AudioListenerIndex	= 0;

	GUID_Lambda								m_ToneSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pToneSoundEffect		= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pToneSoundInstance	= nullptr;

	GUID_Lambda								m_GunSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pGunSoundEffect		= nullptr;

	LambdaEngine::IReverbSphere*			m_pReverbSphere			= nullptr;
	LambdaEngine::IAudioGeometry*			m_pAudioGeometry		= nullptr;

	LambdaEngine::Scene*					m_pScene				= nullptr;
	LambdaEngine::Camera*					m_pCamera				= nullptr;
	LambdaEngine::Sampler*					m_pLinearSampler		= nullptr;
	LambdaEngine::Sampler*					m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*				m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*					m_pRenderer				= nullptr;

	LambdaEngine::RenderGraphEditor*		m_pRenderGraphEditor	= nullptr;

	LambdaEngine::TArray<InstanceIndexAndTransform>		m_InstanceIndicesAndTransforms;
	LambdaEngine::TArray<InstanceIndexAndTransform>		m_LightInstanceIndicesAndTransforms;

	float									m_DirectionalLightAngle;
	float									m_DirectionalLightStrength[4];

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;
};
