#include "Game/ECS/Components/Physics/Transform.h"

#include "Log/Log.h"

#include <cmath>

namespace LambdaEngine
{
    INIT_COMPONENT(PositionComponent);
    INIT_COMPONENT(ScaleComponent);
    INIT_COMPONENT(RotationComponent);
    INIT_COMPONENT(WorldMatrixComponent);

    float GetPitch(const glm::vec3& forward)
    {
        glm::vec3 right = glm::normalize(glm::cross(forward, g_DefaultForward));
        return glm::orientedAngle(g_DefaultForward, forward, right);
    }

    void RotateAroundPoint(const glm::vec3& P, glm::vec3& V, const glm::vec3& axis, float angle)
    {
        V = V - P;
        V = glm::rotate(glm::angleAxis(angle, axis), V);
        V = V + P;
    }
}
