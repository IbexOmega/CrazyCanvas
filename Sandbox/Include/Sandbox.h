#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"
#include "Application/API/PlatformApplication.h"

#include "Containers/TArray.h"

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
	class ISampler;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::EventHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

    // Inherited via IEventHandler
    virtual void FocusChanged(LambdaEngine::IWindow* pWindow, bool hasFocus)                                                 override;
    virtual void WindowMoved(LambdaEngine::IWindow* pWindow, int16 x, int16 y)                                               override;
    virtual void WindowResized(LambdaEngine::IWindow* pWindow, uint16 width, uint16 height, LambdaEngine::EResizeType type)  override;
    virtual void WindowClosed(LambdaEngine::IWindow* pWindow)                                                                override;
    virtual void MouseEntered(LambdaEngine::IWindow* pWindow)                                                                override;
    virtual void MouseLeft(LambdaEngine::IWindow* pWindow)                                                                   override;

	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;
	virtual void KeyReleased(LambdaEngine::EKey key)                                        override;
	virtual void KeyTyped(uint32 character)                                                 override;
	
	virtual void MouseMoved(int32 x, int32 y)                                               override;
	virtual void ButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)      override;
	virtual void ButtonReleased(LambdaEngine::EMouseButton button)                          override;
    virtual void MouseScrolled(int32 deltaX, int32 deltaY)                                  override;
    
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

private:
	bool InitRendererForDeferred();
	bool InitRendererForVisBuf();

private:
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
	LambdaEngine::ISampler*					m_pLinearSampler		= nullptr;
	LambdaEngine::ISampler*					m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*				m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*					m_pRenderer				= nullptr;

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

};
