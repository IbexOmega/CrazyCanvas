#include "Game/ECS/Components/Physics/Transform.h"

#include "Log/Log.h"

#include <cmath>

namespace LambdaEngine
{
    INIT_COMPONENT(PositionComponent);
    INIT_COMPONENT(ScaleComponent);
    INIT_COMPONENT(RotationComponent);
    INIT_COMPONENT(WorldMatrixComponent);

    TransformHandler::TransformHandler()
        :ComponentHandler(TID(TransformHandler))
    {
        ComponentHandlerRegistration handlerReg = {};
        handlerReg.ComponentRegistrations = {
            {PositionComponent::s_TID,     &m_Positions},
            {ScaleComponent::s_TID,        &m_Scales},
            {RotationComponent::s_TID,     &m_Rotations},
            {WorldMatrixComponent::s_TID,  &m_WorldMatrices}
        };

        RegisterHandler(handlerReg);
    }

    bool TransformHandler::InitHandler()
    {
        return true;
    }

    void TransformHandler::CreatePosition(Entity entity, const glm::vec3& position)
    {
        m_Positions.PushBack({ position }, entity);
        RegisterComponent(entity, PositionComponent::s_TID);
    }

    void TransformHandler::CreateScale(Entity entity, const glm::vec3& scale)
    {
        m_Scales.PushBack({ scale }, entity);
        RegisterComponent(entity, ScaleComponent::s_TID);
    }

    void TransformHandler::CreateRotation(Entity entity)
    {
        m_Rotations.PushBack({ glm::identity<glm::quat>() }, entity);
        RegisterComponent(entity, RotationComponent::s_TID);
    }

    void TransformHandler::CreateTransform(Entity entity, const glm::vec3& position, const glm::vec3& scale)
    {
        CreatePosition(entity, position);
        CreateScale(entity, scale);
        CreateRotation(entity);
    }

    void TransformHandler::CreateWorldMatrix(Entity entity, const Transform& transform)
    {
        WorldMatrixComponent worldMatrix = {
            .WorldMatrix =
                glm::scale(glm::mat4(), glm::vec3(transform.Scale)) *
                glm::toMat4(transform.RotationQuaternion) *
                glm::translate(glm::mat4(), transform.Position),
            .Dirty = false
        };

        m_WorldMatrices.PushBack(worldMatrix, entity);
        RegisterComponent(entity, WorldMatrixComponent::s_TID);
    }

    Transform TransformHandler::GetTransform(Entity entity)
    {
        return {
            m_Positions.IndexID(entity).Position,
            m_Scales.IndexID(entity).Scale,
            m_Rotations.IndexID(entity).Quaternion
        };
    }

    WorldMatrixComponent& TransformHandler::GetWorldMatrix(Entity entity)
    {
        WorldMatrixComponent& worldMatrix = m_WorldMatrices.IndexID(entity);

        if (worldMatrix.Dirty)
        {
            const Transform transform = GetTransform(entity);

            worldMatrix = {
                .WorldMatrix =
                    glm::scale(glm::mat4(), glm::vec3(transform.Scale)) *
                    glm::toMat4(transform.RotationQuaternion) *
                    glm::translate(glm::mat4(), transform.Position),
                .Dirty = false
            };
        }

        return worldMatrix;
    }

    float TransformHandler::GetPitch(const glm::vec3& forward)
    {
        glm::vec3 right = glm::normalize(glm::cross(forward, g_DefaultForward));
        return glm::orientedAngle(g_DefaultForward, forward, right);
    }

    void TransformHandler::RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle)
    {
        V = V - P;
        V = glm::rotate(glm::angleAxis(angle, axis), V);
        V = V + P;
    }
}
