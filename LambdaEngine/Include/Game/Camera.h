#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class CommonApplication;

	class LAMBDA_API Camera
	{
	public:
		Camera();
		~Camera() = default;

		//void Init(const CameraDesc& desc);

		void SetDirection(const glm::vec3& direction);
		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);

		void Rotate(const glm::vec3& rotation);
		void Translate(const glm::vec3& translation);

		void Update();
		void HandleInput(Timestamp delta);

		//FORCEINLINE const CameraData&	GetData()					const	{ return m_Data;			}
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
		//CameraData m_Data;

		float m_SpeedFactor = 0.05f;
		bool m_Toggle = false;
		bool m_IsKeyPressed = false;

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

