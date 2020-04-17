#pragma once

#include "FMOD.h"
#include "Audio/API/IAudioGeometry.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class AudioGeometryFMOD : public IAudioGeometry
	{
	public:
		AudioGeometryFMOD(const IAudioDevice* pAudioDevice);
		~AudioGeometryFMOD();

		virtual bool Init(const AudioGeometryDesc& desc) override final;
		virtual void SetActive(bool active) override final;
		virtual void SetPosition(const glm::vec3& position) override final;
		virtual void SetRotation(const glm::vec3& forward, const glm::vec3& up) override final;
		virtual void SetScale(const glm::vec3& scale) override final;
		virtual const glm::vec3& GetPosition() override final;
		virtual const glm::vec3& GetForward() override final;
		virtual const glm::vec3& GetUp() override final;
		virtual const glm::vec3& GetScale() override final;
		
	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//FMOD
		FMOD_GEOMETRY*		m_pGeometry;

		//Locals
		const char*			m_pName;
		glm::vec3			m_Position;
		glm::vec3			m_Forward;
		glm::vec3			m_Up;
		glm::vec3			m_Scale;
	};
}