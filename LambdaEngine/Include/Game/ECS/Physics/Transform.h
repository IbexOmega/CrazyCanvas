#pragma once

#include "Containers/IDVector.h"
#include "ECS/ComponentHandler.h"
#include "ECS/EntitySubscriber.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	const glm::vec3 g_DefaultForward = glm::vec3(0.0f, 0.0f, 1.0f);
	//The up vector is inverted because of vulkans inverted y-axis
	const glm::vec3 g_DefaultUp = glm::vec3(0.0f, -1.0f, 0.0f);

	struct Position
	{
		glm::vec3 Position;
	};

	struct Scale
	{
		glm::vec3 Scale;
	};

	struct Rotation
	{
		glm::quat Quaternion;
	};

	struct DirtyTransformFlag
	{
		bool IsDirty;
	};

	const std::type_index g_TIDPosition	= TID(Position);
	const std::type_index g_TIDScale	= TID(Scale);
	const std::type_index g_TIDRotation	= TID(Rotation);
	const std::type_index g_TIDDirtyTransformFlag = TID(DirtyTransformFlag);

	// Transform is a convenience wrapper
	struct Transform
	{
		glm::vec3& Position;
		glm::vec3& Scale;
		glm::quat& RotationQuaternion;
	};

	struct WorldMatrix
	{
		glm::mat4 WorldMatrix;
	};

	const std::type_index g_TIDWorldMatrix = TID(WorldMatrix);

	class TransformHandler : public ComponentHandler
	{
	public:
		DECL_STATIC_CLASS(TransformHandler);

		bool Init();
		virtual bool InitHandler() override;

		void CreatePosition(Entity entity, const glm::vec3& position = {0.0f, 0.0f, 0.0f});
		void CreateScale(Entity entity, const glm::vec3& scale = {1.0f, 1.0f, 1.0f});
		void CreateRotation(Entity entity);

		void CreateTransform(Entity entity, const glm::vec3& position = {0.0f, 0.0f, 0.0f}, const glm::vec3& scale = {1.0f, 1.0f, 1.0f});
		// Requires that the entity has a transform component
		void CreateWorldMatrix(Entity entity, const Transform& transform);

		// Required components: Position, Rotation and Scale
		Transform GetTransform(Entity entity);
		// Required components: Position, Rotation, Scale and World Matrix
		WorldMatrix& GetWorldMatrix(Entity entity) { return m_WorldMatrices.IndexID(entity); }
		glm::vec3& GetPosition(Entity entity);
		glm::vec3& GetScale(Entity entity);
		glm::quat& GetRotation(Entity entity);

		const glm::vec3& GetPosition(Entity entity) const	{ return m_Positions.IndexID(entity).Position; }
		const glm::vec3& GetScale(Entity entity) const		{ return m_Scales.IndexID(entity).Scale; }
		const glm::quat& GetRotation(Entity entity) const	{ return m_Rotations.IndexID(entity).Quaternion; }

	public:
		static TransformHandler* GetInstance() { return &s_Instance; }

		// Transform calculation functions
		static glm::vec3 GetUp(const glm::quat& rotationQuat)				{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultUp)); }
		static glm::vec3 GetForward(const glm::quat& rotationQuat)			{ return glm::normalize(glm::rotate(rotationQuat, g_DefaultForward)); }
		static glm::vec3 GetRight(const glm::quat& rotationQuat)			{ return glm::normalize(glm::cross(GetForward(rotationQuat), GetUp(rotationQuat))); }
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
		// Called when a transform is created
		void CreateTransformDirtyFlag(Entity entity);

	private:
		static TransformHandler s_Instance;

		IDDVector<Position>	m_Positions;
		IDDVector<Scale>	m_Scales;
		IDDVector<Rotation>	m_Rotations;
		IDDVector<DirtyTransformFlag> m_DirtyTransformFlags;
		IDDVector<WorldMatrix> m_WorldMatrices;
	};
}
