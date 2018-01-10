#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include "geometric/compgeom.h"

float glm::cross(const glm::vec2& v1, const glm::vec2& v2)
{
    return v1.x * v2.y - v2.x * v1.y;
}

float glm::angleFromTo(const glm::vec2& v1, const glm::vec2& v2)
{
    float angle = glm::dot(v1, v2) / (glm::length(v1) * glm::length(v2));
    angle = glm::acos(glm::clamp(angle, -1.0f, 1.0f));
    if(glm::rightTurn(v1, v2))
        angle *= -1.0f;
    return angle;
}

float glm::angleBetween(const glm::vec3& v1, const glm::vec3& v2)
{
    return glm::acos(glm::clamp(glm::dot(v1, v2) / (glm::length(v1) * glm::length(v2)), -1.0f, 1.0f));
}

bool glm::rightTurn(const glm::vec2& v1, const glm::vec2& v2)
{
    return glm::cross(v1, v2) < 0.0f;
}

bool glm::leftTurn(const glm::vec2& v1, const glm::vec2& v2)
{
    return glm::cross(v1, v2) > 0.0f;
}

glm::mat2 glm::rotation(float angle)
{
    return glm::mat2(glm::vec2(glm::cos(angle), glm::sin(angle)),
                     glm::vec2(-glm::sin(angle), glm::cos(angle)));
}

glm::mat3 glm::transformation(const glm::vec2& translation, float rotation)
{
    glm::mat3 mx;
    mx[0] = glm::vec3( glm::cos(rotation), glm::sin(rotation), 0.0f);
    mx[1] = glm::vec3(-glm::sin(rotation), glm::cos(rotation), 0.0f);
    mx[2] = glm::vec3( translation.x,      translation.y,      1.0f);
    return mx;
}
