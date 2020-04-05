#pragma once

#include "LambdaEngine.h"

#include "Math/Math.h"

struct FMOD_GEOMETRY;
struct FMOD_VECTOR;

namespace LambdaEngine
{
	struct Mesh;

	class AudioDevice;

	struct AudioMeshParameters
	{
		float DirectOcclusion	= 0;
		float ReverbOcclusion	= 0;
		bool DoubleSided		= false;
	};

	struct AudioGeometryDesc
	{
		const char* pName										= "AudioGeometry";
		const Mesh* const * ppMeshes							= nullptr;
		const glm::mat4* pTransforms							= nullptr;
		const AudioMeshParameters*	pAudioMeshParameters		= nullptr;
		uint32 NumMeshes										= 0;
	};

	class LAMBDA_API AudioGeometry
	{
	public:
		DECL_REMOVE_COPY(AudioGeometry);
		DECL_REMOVE_MOVE(AudioGeometry);

		AudioGeometry(const AudioDevice* pAudioDevice);
		~AudioGeometry();

		/*
		* Initialize this AudioGeometry
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const AudioGeometryDesc& desc);

		/*
		* Set whether the geometry should be processed by the audio engine
		*/
		void SetActive(bool active);

		/*
		* Set the world position of the geometry
		*	position - The world space position, should be given in meters
		*/
		void SetPosition(const glm::vec3& position);

		/*
		* Set the world rotation of the geometry
		*	forward - The forward facing vector, assumed to be normalized
		*	up - The upwards pointing vector, assumed to be normalized
		*/
		void SetRotation(const glm::vec3& forward, const glm::vec3& up);

		/*
		* Set the world scale of the geometry
		*	scale - The world space scale
		*/
		void SetScale(const glm::vec3& scale);

		const glm::vec3& GetPosition();
		const glm::vec3& GetForward();
		const glm::vec3& GetUp();
		const glm::vec3& GetScale();

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;

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