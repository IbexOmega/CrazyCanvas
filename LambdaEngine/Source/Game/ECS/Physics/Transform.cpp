#include "Game/ECS/Physics/Transform.h"

#include "ECS/ECSCore.h"
#include "Log/Log.h"

#include <cmath>

namespace LambdaEngine
{
    TransformHandler TransformHandler::s_Instance;

    bool TransformHandler::Init()
    {
        ComponentHandlerRegistration handlerReg = {};
        handlerReg.ComponentRegistrations = {
            {g_TIDPosition,     &m_Positions},
            {g_TIDScale,        &m_Scales},
            {g_TIDRotation,     &m_Rotations},
            {g_TIDWorldMatrix,  &m_WorldMatrices}
        };

        RegisterHandler(handlerReg);
        SetHandlerType(TID(TransformHandler));
        return true;
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
            glm::scale(glm::mat4(), glm::vec3(transform.Scale)) *
            glm::toMat4(transform.RotationQuaternion) *
            glm::translate(glm::mat4(), transform.Position),
        };

        m_WorldMatrices.PushBack(worldMatrix, entity);
        RegisterComponent(entity, g_TIDWorldMatrix);
    }

    Transform TransformHandler::GetTransform(Entity entity)
    {
        if (m_DirtyTransformFlags.HasElement(entity))
            m_DirtyTransformFlags.IndexID(entity).IsDirty = true;

        return {
            m_Positions.IndexID(entity).Position,
            m_Scales.IndexID(entity).Scale,
            m_Rotations.IndexID(entity).Quaternion
        };
    }

    glm::vec3& TransformHandler::GetPosition(Entity entity)
    {
        if (m_DirtyTransformFlags.HasElement(entity))
            m_DirtyTransformFlags.IndexID(entity).IsDirty = true;

        return m_Positions.IndexID(entity).Position;
    }

    glm::vec3& TransformHandler::GetScale(Entity entity)
    {
        if (m_DirtyTransformFlags.HasElement(entity))
            m_DirtyTransformFlags.IndexID(entity).IsDirty = true;

        return m_Scales.IndexID(entity).Scale;
    }

    glm::quat& TransformHandler::GetRotation(Entity entity)
    {
        if (m_DirtyTransformFlags.HasElement(entity))
            m_DirtyTransformFlags.IndexID(entity).IsDirty = true;

        return m_Rotations.IndexID(entity).Quaternion;
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

    void TransformHandler::CreateTransformDirtyFlag(Entity entity)
    {
        // Schedule a job that creates the dirty flag component, to ensure that the component creation is free from data races
        Job job = {
            .Function = [this, entity] {
                m_DirtyTransformFlags.PushBack({ true }, entity);
                RegisterComponent(entity, g_TIDDirtyTransformFlag);
            },
            .Components = {
                {RW, g_TIDDirtyTransformFlag}
            }
        };

        ECSCore::GetInstance()->ScheduleJobASAP(job);
    }
}
