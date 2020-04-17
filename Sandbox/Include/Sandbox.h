#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	struct GameObject;

	class RenderGraph;
	class Renderer;
	class ResourceManager;
	class IAudioListener;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	class Scene;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IMouseHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

	// Inherited via IMouseHandler
	virtual void OnMouseMove(int32 x, int32 y)                          override;
	virtual void OnButtonPressed(LambdaEngine::EMouseButton button)     override;
	virtual void OnButtonReleased(LambdaEngine::EMouseButton button)    override;
	virtual void OnScroll(int32 delta)                                  override;

private:
	LambdaEngine::ResourceManager*			m_pResourceManager;

	GUID_Lambda								m_ToneSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pToneSoundEffect;
	LambdaEngine::ISoundInstance3D*			m_pToneSoundInstance;

	GUID_Lambda								m_GunSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pGunSoundEffect;

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

	LambdaEngine::IAudioListener*			m_pAudioListener;

	LambdaEngine::IReverbSphere*			m_pReverbSphere;
	LambdaEngine::IAudioGeometry*			m_pAudioGeometry;

	LambdaEngine::Scene*					m_pScene;
	LambdaEngine::RenderGraph*				m_pRenderGraph;
	LambdaEngine::Renderer*					m_pRenderer;
};
