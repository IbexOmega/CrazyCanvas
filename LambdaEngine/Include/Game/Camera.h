#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class CommonApplication;

	struct CameraData
	{
		glm::mat4 Projection		= glm::mat4(1.0f);
		glm::mat4 View				= glm::mat4(1.0f);
		glm::mat4 PrevProjection	= glm::mat4(1.0f);
		glm::mat4 PrevView			= glm::mat4(1.0f);
		glm::mat4 ViewInv			= glm::mat4(1.0f);
		glm::mat4 ProjectionInv		= glm::mat4(1.0f);
		glm::vec4 Position			= glm::vec4(0.0f);
		glm::vec4 Right				= glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec4 Up				= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec2 Jitter			= glm::vec2(0.0f);
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

		void Init(CommonApplication* commonApplication, const CameraDesc& desc);

		void SetDirection(const glm::vec3& direction);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);

		void Rotate(const glm::vec3& rotation);
		void Translate(const glm::vec3& translation);

		void Update();
		void HandleInput(Timestamp delta);

		FORCEINLINE const CameraData&	GetData()					const	{ return m_Data;			}
		FORCEINLINE const glm::mat4&	GetProjectionMat()			const	{ return m_Projection;		}
		FORCEINLINE const glm::mat4&	GetProjectionInvMat()		const	{ return m_ProjectionInv;	}
		FORCEINLINE const glm::mat4&	GetViewMat()				const	{ return m_View;			}
		FORCEINLINE const glm::mat4&	GetViewInvMat()				const	{ return m_ViewInv;			}
		FORCEINLINE const glm::vec3&	GetPosition()				const	{ return m_Position;		}
		FORCEINLINE const glm::vec3&	GetRotation()				const	{ return m_Rotation;		}
		FORCEINLINE const glm::vec3&	GetForwardVec()				const	{ return m_Forward;			}
		FORCEINLINE const glm::vec3&	GetRightVec()				const	{ return m_Right;			}
		FORCEINLINE const glm::vec3&	GetUpVec()					const	{ return m_Up;				}
		FORCEINLINE const float			GetFOVDegrees()				const	{ return m_FOVDegrees;		}
		FORCEINLINE const float			GetWidth()					const	{ return m_Width;			}
		FORCEINLINE const float			GetHeight()					const	{ return m_Height;			}
		FORCEINLINE const float			GetNearPlane()				const	{ return m_NearPlane;		}
		FORCEINLINE const float			GetFarPlane()				const	{ return m_FarPlane;		}

	private:
		void CalculateVectors();

	private:
		CommonApplication* m_CommonApplication;
		CameraData m_Data;

		float m_SpeedFactor = 0.05f;

		glm::mat4 m_Projection;
		glm::mat4 m_ProjectionInv;
		glm::mat4 m_View;
		glm::mat4 m_ViewInv;

		glm::vec3 m_Position;
		glm::vec3 m_Rotation;

		glm::vec3 m_Forward;
		glm::vec3 m_Right;
		glm::vec3 m_Up;

		float m_FOVDegrees;
		float m_Width;
		float m_Height;
		float m_NearPlane;
		float m_FarPlane;

		bool m_IsDirty;
		bool m_LastIsDirty;
	};
}

