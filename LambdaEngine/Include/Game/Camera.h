#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct CameraData
	{
		glm::mat4 Projection		= glm::mat4(1.0f);
		glm::mat4 View				= glm::mat4(1.0f);
		glm::mat4 LastProjection	= glm::mat4(1.0f);
		glm::mat4 LastView			= glm::mat4(1.0f);
		glm::mat4 ViewInv			= glm::mat4(1.0f);
		glm::mat4 ProjectionInv		= glm::mat4(1.0f);
		glm::vec4 Position			= glm::vec4(0.0f);
		glm::vec4 Right				= glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec4 Up				= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	};

	struct CameraDesc
	{
		glm::vec3 Position	= glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Direction = glm::vec3(1.0f, 0.0f, 0.0f);
		float FOVDegrees	= 45.0f;
		float Width			= 1280.0f;
		float Height		= 720.0f; 
		float NearPlane		= 0.0001f; 
		float FarPlane		= 50.0f;
	};

	class LAMBDA_API Camera
	{
	public:
		Camera();
		~Camera() = default;

		void Init(const CameraDesc& desc);

		void SetDirection(const glm::vec3& direction);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);

		void Rotate(const glm::vec3& rotation);
		void Translate(const glm::vec3& translation);

		void Update();

		FORCEINLINE const CameraData& GetData()					const	{ return m_Data;			}
		FORCEINLINE const glm::mat4& GetProjectionMat()			const	{ return m_Projection;		}
		FORCEINLINE const glm::mat4& GetProjectionInvMat()		const	{ return m_ProjectionInv;	}
		FORCEINLINE const glm::mat4& GetViewMat()				const	{ return m_View;			}
		FORCEINLINE const glm::mat4& GetViewInvMat()			const	{ return m_ViewInv;			}
		FORCEINLINE const glm::vec3& GetPosition()				const	{ return m_Position;		}
		FORCEINLINE const glm::vec3& GetRotation()				const	{ return m_Rotation;		}
		FORCEINLINE const glm::vec3& GetForwardVec()			const	{ return m_Forward;			}
		FORCEINLINE const glm::vec3& GetRightVec()				const	{ return m_Right;			}
		FORCEINLINE const glm::vec3& GetUpVec()					const	{ return m_Up;				}

	private:
		void CalculateVectors();

	private:
		CameraData m_Data;

		glm::mat4 m_Projection;
		glm::mat4 m_ProjectionInv;
		glm::mat4 m_View;
		glm::mat4 m_ViewInv;

		glm::vec3 m_Position;
		glm::vec3 m_Rotation;

		glm::vec3 m_Forward;
		glm::vec3 m_Right;
		glm::vec3 m_Up;

		bool m_IsDirty;
		bool m_LastIsDirty;
	};
}

