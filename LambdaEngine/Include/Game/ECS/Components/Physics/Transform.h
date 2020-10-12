#pragma once

#include "ECS/Component.h"
#include "ECS/EntitySubscriber.h"
#include "Math/Math.h"

#define GRAVITATIONAL_ACCELERATION 9.81f

namespace LambdaEngine
{
	const glm::vec3 g_DefaultForward	= glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 g_DefaultRight		= glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 g_DefaultUp 		= glm::vec3(0.0f, 1.0f, 0.0f);

	struct PositionComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(PositionComponent);
		glm::vec3 Position;
	};

	struct ScaleComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(ScaleComponent);
		glm::vec3 Scale;
	};

	struct RotationComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(RotationComponent);
		glm::quat Quaternion;
	};

	// Transform is a convenience wrapper
	struct Transform
	{
		glm::vec3& Position;
		glm::vec3& Scale;
		glm::quat& RotationQuaternion;
	};

	struct VelocityComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(VelocityComponent);
		glm::vec3 Velocity = glm::vec3(0.0f);
	};

	struct OffsetComponent
	{
		DECL_COMPONENT(OffsetComponent);
		glm::vec3 Offset = glm::vec3(0.0f);
	};

	class TransformComponents : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final
		{
			return { Position, Scale, Rotation };
		}

		GroupedComponent<PositionComponent>	Position;
		GroupedComponent<ScaleComponent>	Scale;
		GroupedComponent<RotationComponent>	Rotation;
	};

	// Transform calculation functions
	inline glm::vec3 GetUp(const glm::quat& rotationQuat)				{ return glm::rotate(rotationQuat, g_DefaultUp); }
	inline glm::vec3 GetForward(const glm::quat& rotationQuat)			{ return glm::rotate(rotationQuat, g_DefaultForward); }
	inline glm::vec3 GetRight(const glm::quat& rotationQuat)			{ return glm::rotate(rotationQuat, g_DefaultRight); }
	inline glm::quat GetRotationQuaternion(const glm::vec3& forward)	{ return glm::rotation(forward, g_DefaultForward); }

	float GetPitch(const glm::vec3& forward);
	float GetYaw(const glm::vec3& forward);
	inline float GetRoll(const glm::quat& rotationQuat)	{ return glm::orientedAngle(g_DefaultUp, GetUp(rotationQuat), GetForward(rotationQuat)); }

	// Assumes the parameters are normalized
	inline void SetForward(glm::quat& rotationQuat, const glm::vec3& forward) { rotationQuat = glm::rotation(GetForward(rotationQuat), forward); }

	inline void Roll(glm::quat& rotationQuat, float angle) { rotationQuat = glm::rotate(rotationQuat, angle, GetForward(rotationQuat)); };

	// Rotate V around P using a given axis and angle
	void RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle);
}
