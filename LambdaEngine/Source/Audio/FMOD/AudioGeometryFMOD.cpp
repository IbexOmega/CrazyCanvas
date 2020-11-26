#include "Audio/FMOD/AudioGeometryFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Resources/Mesh.h"
#include "Log/Log.h"

namespace LambdaEngine
{
	struct FMODInitPolygon
	{
		FMOD_VECTOR Triangle[3];
		uint32 MeshIndex;
	};

	AudioGeometryFMOD::AudioGeometryFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pGeometry(nullptr)
	{
	}

	AudioGeometryFMOD::~AudioGeometryFMOD()
	{
		if (m_pGeometry != nullptr)
		{
			if (FMOD_Geometry_Release(m_pGeometry) != FMOD_OK)
			{
				LOG_WARNING("[AudioGeometryFMOD]: FMOD Geometry could not be released for %s", m_pName);
			}

			m_pGeometry = nullptr;
		}
	}

	bool AudioGeometryFMOD::Init(const AudioGeometryDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;

		uint32 numVertices = 0;
		TArray<FMODInitPolygon> polygons;

		for (uint32 m = 0; m < pDesc->NumMeshes; m++)
		{
			const Mesh* pMesh = pDesc->ppMeshes[m];
			glm::mat4 transform = pDesc->pTransforms != nullptr ? pDesc->pTransforms[m] : glm::mat4(1.0f);

			numVertices += pMesh->Indices.GetSize();
			for (uint32 i = 0; i < pMesh->Indices.GetSize(); i += 3)
			{
				glm::vec3 v0Pos = transform * glm::vec4(pMesh->Vertices[pMesh->Indices[i + 0]].Position, 1.0f);
				glm::vec3 v1Pos = transform * glm::vec4(pMesh->Vertices[pMesh->Indices[i + 1]].Position, 1.0f);
				glm::vec3 v2Pos = transform * glm::vec4(pMesh->Vertices[pMesh->Indices[i + 2]].Position, 1.0f);

				FMOD_VECTOR fmodV0 = { v0Pos.x, v0Pos.y, v0Pos.z };
				FMOD_VECTOR fmodV1 = { v1Pos.x, v1Pos.y, v1Pos.z };
				FMOD_VECTOR fmodV2 = { v2Pos.x, v2Pos.y, v2Pos.z };

                polygons.PushBack({ {fmodV0, fmodV1, fmodV2}, m });
			}
		}

		if (FMOD_System_CreateGeometry(m_pAudioDevice->pSystem, numVertices * 3, numVertices, &m_pGeometry) != FMOD_OK)
		{
			LOG_WARNING("[AudioGeometryFMOD]: Geometry %s could not be created!", m_pName);
			return false;
		}

		for (uint32 t = 0; t < polygons.GetSize(); t++)
		{
			FMODInitPolygon& polygon = polygons[t];
			FMOD_VECTOR* pTriangle = polygon.Triangle;
			const AudioMeshParameters& audioMeshParameters = pDesc->pAudioMeshParameters[polygon.MeshIndex];

			if (FMOD_Geometry_AddPolygon(m_pGeometry, audioMeshParameters.DirectOcclusion, audioMeshParameters.ReverbOcclusion, audioMeshParameters.DoubleSided ? 1 : 0, 3, pTriangle, nullptr) != FMOD_OK)
			{
				LOG_WARNING("[AudioGeometryFMOD]: Polygon number %u could not be added to %s!", t, m_pName);
				return false;
			}
		}

		LOG_DEBUG("[AudioGeometryFMOD]: Successfully initialized %s!", m_pName);

		return true;
	}

	void AudioGeometryFMOD::SetActive(bool active)
	{
		FMOD_Geometry_SetActive(m_pGeometry, active ? 1 : 0);
	}

	void AudioGeometryFMOD::SetPosition(const glm::vec3& position)
	{
		FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };

		FMOD_Geometry_SetPosition(m_pGeometry, &fmodPosition);
	}

	void AudioGeometryFMOD::SetRotation(const glm::vec3& forward, const glm::vec3& up)
	{
		FMOD_VECTOR fmodForward = { forward.x, forward.y, forward.z };
		FMOD_VECTOR fmodUp = { up.x, up.y, up.z };

		FMOD_Geometry_SetRotation(m_pGeometry, &fmodForward, &fmodUp);
	}

	void AudioGeometryFMOD::SetScale(const glm::vec3& scale)
	{
		FMOD_VECTOR fmodScale = { scale.x, scale.y, scale.z };

		FMOD_Geometry_SetScale(m_pGeometry, &fmodScale);
	}

	const glm::vec3& AudioGeometryFMOD::GetPosition()
	{
		return m_Position;
	}

	const glm::vec3& AudioGeometryFMOD::GetForward()
	{
		return m_Forward;
	}

	const glm::vec3& AudioGeometryFMOD::GetUp()
	{
		return m_Up;
	}

	const glm::vec3& AudioGeometryFMOD::GetScale()
	{
		return m_Scale;
	}
}
