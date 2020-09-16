#pragma once

#include "Containers/IDVector.h"
#include "ECS/Component.h"
#include "ECS/ComponentHandler.h"
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

	class TransformHandler : public ComponentHandler
	{
	public:
		TransformHandler();
		~TransformHandler() = default;

		virtual bool InitHandler() override;

		void CreatePosition(Entity entity, const glm::vec3& position = {0.0f, 0.0f, 0.0f});
		void CreateScale(Entity entity, const glm::vec3& scale = {1.0f, 1.0f, 1.0f});
		void CreateRotation(Entity entity);

		void CreateTransform(Entity entity, const glm::vec3& position = {0.0f, 0.0f, 0.0f}, const glm::vec3& scale = {1.0f, 1.0f, 1.0f});
		// Requires that the entity has a transform component
		void CreateWorldMatrix(Entity entity, const Transform& transform);

	public:
		// Required components: Position, Rotation and Scale
		Transform GetTransform(Entity entity);
		// Required components: Position, Rotation, Scale and World Matrix
		WorldMatrixComponent& GetWorldMatrix(Entity entity);
		glm::vec3& GetPosition(Entity entity)   { return m_Positions.IndexID(entity).Position; }
		glm::vec3& GetScale(Entity entity)      { return m_Scales.IndexID(entity).Scale; }
		glm::quat& GetRotation(Entity entity)   { return m_Rotations.IndexID(entity).Quaternion; }

	public:
		// Transform calculation functions
		static glm::vec3 GetUp(const glm::quat& rotationQuat)				{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultUp)); }
		static glm::vec3 GetForward(const glm::quat& rotationQuat)			{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultForward)); }
		static glm::quat GetRotationQuaternion(const glm::vec3& forward)	{ return glm::rotation(forward, g_DefaultForward); }

		static float GetPitch(const glm::vec3& forward);
		static float GetYaw(const glm::vec3& forward)		{ return glm::orientedAngle(g_DefaultForward, forward, g_DefaultUp); }
		static float GetRoll(const glm::quat& rotationQuat)	{ return glm::orientedAngle(g_DefaultUp, GetUp(rotationQuat), GetForward(rotationQuat)); }

		// Assumes the forward is normalized
		static void SetForward(glm::quat& rotationQuat, const glm::vec3& forward) { rotationQuat = glm::rotation(GetForward(rotationQuat), forward); }

		static void Roll(glm::quat& rotationQuat, float angle) { rotationQuat = glm::rotate(rotationQuat, angle, GetForward(rotationQuat)); };

		// Rotate V around P using a given axis and angle
		static void RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle);

	private:
		IDDVector<PositionComponent>     m_Positions;
		IDDVector<ScaleComponent>        m_Scales;
		IDDVector<RotationComponent>     m_Rotations;
		IDDVector<WorldMatrixComponent>  m_WorldMatrices;
	};
}
