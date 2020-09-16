#pragma once

#include "Application/API/Events/WindowEvents.h"
#include "ECS/System.h"

namespace LambdaEngine
{
	class CameraHandler;
	class TransformHandler;

	class CameraController : public System
	{
	public:
		CameraController();
		~CameraController();

		bool InitSystem() override final;

		void Tick(float dt);

	private:
		bool OnScreenResize(const WindowResizedEvent& event);

	private:
		IDVector m_Cameras;

		CameraHandler* m_pCameraHandler;
		TransformHandler* m_pTransformHandler;
		glm::uvec2 m_WindowSize;

		bool m_MouseEnabled, m_CIsPressed;
	};
}
