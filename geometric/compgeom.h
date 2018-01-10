#ifndef IVO_COMP_GEOM_H
#define IVO_COMP_GEOM_H
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>

namespace glm
{
float   cross(const vec2& v1, const vec2& v2);
float   angleFromTo(const vec2& v1, const vec2& v2);
float   angleBetween(const vec3& v1, const vec3& v2);
bool    rightTurn(const vec2& v1, const vec2& v2);
bool    leftTurn(const vec2& v1, const vec2& v2);
mat2    rotation(float angleFromTo);
mat3    transformation(const vec2& translation, float rotation);
}

#endif
