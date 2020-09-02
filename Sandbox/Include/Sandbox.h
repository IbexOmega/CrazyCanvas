#pragma once
#include "Game/Game.h"

#include "Application/API/EventHandler.h"

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

class Sandbox : public LambdaEngine::Game, public LambdaEngine::EventHandler
{
	struct InstanceIndexAndTransform
	{
		uint32		InstanceIndex;
		glm::vec3	Position;
		glm::vec4	Rotation;
		glm::vec3	Scale;
	};

public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

    // Inherited via IEventHandler
    virtual void OnFocusChanged(LambdaEngine::Window* pWindow, bool hasFocus)                                                 override;
    virtual void OnWindowMoved(LambdaEngine::Window* pWindow, int16 x, int16 y)                                               override;
    virtual void OnWindowResized(LambdaEngine::Window* pWindow, uint16 width, uint16 height, LambdaEngine::EResizeType type)  override;
    virtual void OnWindowClosed(LambdaEngine::Window* pWindow)                                                                override;
    virtual void OnMouseEntered(LambdaEngine::Window* pWindow)                                                                override;
    virtual void OnMouseLeft(LambdaEngine::Window* pWindow)                                                                   override;

	virtual void OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;
	virtual void OnKeyReleased(LambdaEngine::EKey key)                                        override;
	virtual void OnKeyTyped(uint32 character)                                                 override;
	
	virtual void OnMouseMoved(int32 x, int32 y)                                               override;
	virtual void OnMouseMovedRaw(int32 deltaX, int32 deltaY)                                  override;
	virtual void OnButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)      override;
	virtual void OnButtonReleased(LambdaEngine::EMouseButton button)                          override;
    virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)                                  override;
    
	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	void Render(LambdaEngine::Timestamp delta);

private:
	bool InitRendererForEmpty();
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
	LambdaEngine::Sampler*					m_pLinearSampler		= nullptr;
	LambdaEngine::Sampler*					m_pNearestSampler		= nullptr;

	LambdaEngine::RenderGraph*				m_pRenderGraph			= nullptr;
	LambdaEngine::Renderer*					m_pRenderer				= nullptr;

	LambdaEngine::RenderGraphEditor*		m_pRenderGraphEditor	= nullptr;

	GUID_Lambda								m_ImGuiPixelShaderNormalGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderDepthGUID					= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderMetallicGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderRoughnessGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderEmissiveGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderPackedLocalNormalGUID		= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelLinearZGUID						= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelCompactNormalFloatGUID			= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelShaderEmissionGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelPackedMetallicGUID				= GUID_NONE;
	GUID_Lambda								m_ImGuiPixelPackedRoughnessGUID				= GUID_NONE;

	LambdaEngine::TArray<InstanceIndexAndTransform>		m_InstanceIndicesAndTransforms;
	LambdaEngine::TArray<InstanceIndexAndTransform>		m_LightInstanceIndicesAndTransforms;

	float									m_DirectionalLightAngle;
	float									m_DirectionalLightStrength[4];

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

};
