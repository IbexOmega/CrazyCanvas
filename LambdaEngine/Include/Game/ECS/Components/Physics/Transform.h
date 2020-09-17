#pragma once

#include "ECS/Component.h"
#include "ECS/EntitySubscriber.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	const glm::vec3 g_DefaultForward = glm::vec3(0.0f, 0.0f, 1.0f);
	//The up vector is inverted because of vulkans inverted y-axis
	const glm::vec3 g_DefaultUp = glm::vec3(0.0f, -1.0f, 0.0f);

	struct PositionComponent
	{
		DECL_COMPONENT(PositionComponent);
		glm::vec3 Position;
		bool Dirty;
	};

	struct ScaleComponent
	{
		DECL_COMPONENT(ScaleComponent);
		glm::vec3 Scale;
		bool Dirty;
	};

	struct RotationComponent
	{
		DECL_COMPONENT(RotationComponent);
		glm::quat Quaternion;
		bool Dirty;
	};

	// Transform is a convenience wrapper
	struct Transform
	{
		glm::vec3& Position;
		glm::vec3& Scale;
		glm::quat& RotationQuaternion;
	};

	class TransformComponents : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final
		{
			return { Position, Scale, Rotation };
		}

		ComponentAccess Position    = {R, PositionComponent::s_TID};
		ComponentAccess Scale       = {R, ScaleComponent::s_TID};
		ComponentAccess Rotation    = {R, RotationComponent::s_TID};
	};

	struct WorldMatrixComponent
	{
		DECL_COMPONENT(WorldMatrixComponent);
		glm::mat4 WorldMatrix;
		// Flags whether or not the potential belonging Transform component has been written to since the last access
		bool Dirty;
	};

	// Transform calculation functions
	inline glm::vec3 GetUp(const glm::quat& rotationQuat)				{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultUp)); }
	inline glm::vec3 GetForward(const glm::quat& rotationQuat)			{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultForward)); }
	inline glm::vec3 GetRight(const glm::quat& rotationQuat)			{ return glm::normalize(glm::cross(GetForward(rotationQuat), GetUp(rotationQuat))); }
	inline glm::quat GetRotationQuaternion(const glm::vec3& forward)	{ return glm::rotation(forward, g_DefaultForward); }

	float GetPitch(const glm::vec3& forward);
	inline float GetYaw(const glm::vec3& forward)			{ return glm::orientedAngle(g_DefaultForward, forward, g_DefaultUp); }
	inline float GetRoll(const glm::quat& rotationQuat)	{ return glm::orientedAngle(g_DefaultUp, GetUp(rotationQuat), GetForward(rotationQuat)); }

	// Assumes the forward is normalized
	inline void SetForward(glm::quat& rotationQuat, const glm::vec3& forward) { rotationQuat = glm::rotation(GetForward(rotationQuat), forward); }

	inline void Roll(glm::quat& rotationQuat, float angle) { rotationQuat = glm::rotate(rotationQuat, angle, GetForward(rotationQuat)); };

	// Rotate V around P using a given axis and angle
	void RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle);
}
