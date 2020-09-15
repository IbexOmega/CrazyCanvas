#include "Game/ECS/Components/Physics/Transform.h"

#include "Log/Log.h"

#include <cmath>

namespace LambdaEngine
{
    TransformHandler::TransformHandler()
        :ComponentHandler(TID(TransformHandler))
    {
        ComponentHandlerRegistration handlerReg = {};
        handlerReg.ComponentRegistrations = {
            {g_TIDPosition,     &m_Positions},
            {g_TIDScale,        &m_Scales},
            {g_TIDRotation,     &m_Rotations},
            {g_TIDWorldMatrix,  &m_WorldMatrices}
        };

        RegisterHandler(handlerReg);
    }

    bool TransformHandler::InitHandler()
    {
        return true;
    }

    void TransformHandler::CreatePosition(Entity entity, const glm::vec3& position)
    {
        m_Positions.PushBack({position}, entity);
        RegisterComponent(entity, g_TIDPosition);
    }

    void TransformHandler::CreateScale(Entity entity, const glm::vec3& scale)
    {
        m_Scales.PushBack({scale}, entity);
        RegisterComponent(entity, g_TIDScale);
    }

    void TransformHandler::CreateRotation(Entity entity)
    {
        m_Rotations.PushBack({ glm::identity<glm::quat>() }, entity);
        RegisterComponent(entity, g_TIDRotation);
    }

    void TransformHandler::CreateTransform(Entity entity, const glm::vec3& position, const glm::vec3& scale)
    {
        CreatePosition(entity, position);
        CreateScale(entity, scale);
        CreateRotation(entity);
    }

    void TransformHandler::CreateWorldMatrix(Entity entity, const Transform& transform)
    {
        WorldMatrix worldMatrix = {
            .WorldMatrix =
                glm::scale(glm::mat4(), glm::vec3(transform.Scale)) *
                glm::toMat4(transform.RotationQuaternion) *
                glm::translate(glm::mat4(), transform.Position),
            .Dirty = false
        };

        m_WorldMatrices.PushBack(worldMatrix, entity);
        RegisterComponent(entity, g_TIDWorldMatrix);
    }

    Transform TransformHandler::GetTransform(Entity entity)
    {
        return {
            m_Positions.IndexID(entity).Position,
            m_Scales.IndexID(entity).Scale,
            m_Rotations.IndexID(entity).Quaternion
        };
    }

    WorldMatrix& TransformHandler::GetWorldMatrix(Entity entity)
    {
        WorldMatrix& worldMatrix = m_WorldMatrices.IndexID(entity);

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
