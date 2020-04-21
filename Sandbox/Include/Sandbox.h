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
	class Camera;
	class ISampler;
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
	bool InitRendererForDeferred(uint32 backBufferCount, uint32 maxTexturesPerDescriptorSet);
	bool InitRendererForVisBuf(uint32 backBufferCount, uint32 maxTexturesPerDescriptorSet);

private:
	LambdaEngine::ResourceManager*			m_pResourceManager		= nullptr;

	GUID_Lambda								m_ToneSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pToneSoundEffect		= nullptr;
	LambdaEngine::ISoundInstance3D*			m_pToneSoundInstance	= nullptr;

	GUID_Lambda								m_GunSoundEffectGUID;
	LambdaEngine::ISoundEffect3D*			m_pGunSoundEffect		= nullptr;

	LambdaEngine::IAudioListener*			m_pAudioListener		= nullptr;

	LambdaEngine::IReverbSphere*			m_pReverbSphere			= nullptr;
	LambdaEngine::IAudioGeometry*			m_pAudioGeometry		= nullptr;

	LambdaEngine::Scene*					m_pScene				= nullptr;
	LambdaEngine::Camera*					m_pCamera				= nullptr;
	LambdaEngine::ISampler*					m_pLinearSampler		= nullptr;
	LambdaEngine::ISampler*					m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*				m_pRenderGraph	= nullptr;
	LambdaEngine::Renderer*					m_pRenderer				= nullptr;

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

};
