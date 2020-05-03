#pragma once

#include "LambdaEngine.h"

#include "Math/Math.h"


namespace LambdaEngine
{
	struct Mesh;
	
	struct AudioMeshParameters
	{
		float DirectOcclusion	= 0;
		float ReverbOcclusion	= 0;
		bool DoubleSided		= false;
	};

	struct AudioGeometryDesc
	{
		const char* pName										= "AudioGeometryFMOD";
		const Mesh* const * ppMeshes							= nullptr;
		const glm::mat4* pTransforms							= nullptr;
		const AudioMeshParameters*	pAudioMeshParameters		= nullptr;
		uint32 NumMeshes										= 0;
	};
	
	class IAudioGeometry
	{
	public:
		DECL_INTERFACE(IAudioGeometry);

		/*
		* Initialize this AudioGeometryFMOD
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const AudioGeometryDesc* pDesc) = 0;

		/*
		* Set whether the geometry should be processed by the audio engine
		*/
		virtual void SetActive(bool active) = 0;

		/*
		* Set the world position of the geometry
		*	position - The world space position, should be given in meters
		*/
		virtual void SetPosition(const glm::vec3& position) = 0;

		/*
		* Set the world rotation of the geometry
		*	forward - The forward facing vector, assumed to be normalized
		*	up - The upwards pointing vector, assumed to be normalized
		*/
		virtual void SetRotation(const glm::vec3& forward, const glm::vec3& up) = 0;

		/*
		* Set the world scale of the geometry
		*	scale - The world space scale
		*/
		virtual void SetScale(const glm::vec3& scale) = 0;

		virtual const glm::vec3& GetPosition() = 0;
		virtual const glm::vec3& GetForward() = 0;
		virtual const glm::vec3& GetUp() = 0;
		virtual const glm::vec3& GetScale() = 0;
	};
}