#pragma once

#include "Containers/IDVector.h"
#include "ECS/ComponentHandler.h"
#include "ECS/EntitySubscriber.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct ViewProjectionMatrices
	{
		glm::mat4 Projection		= glm::mat4(1.0f);
		glm::mat4 View				= glm::mat4(1.0f);
		glm::mat4 PrevProjection	= glm::mat4(1.0f);
		glm::mat4 PrevView			= glm::mat4(1.0f);
	};

	struct CameraProperties
	{
		glm::vec2 Jitter = glm::vec2(0.0f);
	};

	const std::type_index g_TIDViewProjectionMatrices = TID(ViewProjectionMatrices);
	const std::type_index g_TIDCameraProperties = TID(ViewProjectionMatrices);

	// Details how to create view and projection matrices
	struct ViewProjectionDesc
	{
		glm::vec3 Position;
		glm::vec3 Direction;
		float FOVDegrees;
		float Width;
		float Height;
		float NearPlane;
		float FarPlane;
	};

	class LAMBDA_API CameraHandler : public ComponentHandler
	{
	public:
		CameraHandler();
		~CameraHandler() = default;

		bool Init();
		bool InitHandler() override final { return true; }

		ViewProjectionMatrices& GetViewProjectionMatrices(Entity entity) { return m_VPMatrices.IndexID(entity); }
		CameraProperties& GetCameraProperties(Entity entity) { return m_CameraProperties.IndexID(entity); }

		void CreateViewProjectionMatrices(Entity entity, const ViewProjectionDesc& matricesDesc);
		void CreateCameraProperties(Entity entity);

		static CameraHandler* GetInstance() { return &s_Instance; }

	private:
		static CameraHandler s_Instance;

		IDDVector<ViewProjectionMatrices> m_VPMatrices;
		IDDVector<CameraProperties> m_CameraProperties;
	};
}
