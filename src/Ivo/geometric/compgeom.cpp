/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include "geometric/compgeom.h"

float glm::sign(float f)
{
    if(f < 0.0f)
        return -1.0f;
    if(f > 0.0f)
        return 1.0f;
    return 0.0f;
}

int glm::crossSign(const glm::ivec2& v1, const glm::ivec2& v2)
{
    int64_t v1x = static_cast<int64_t>(v1.x);
    int64_t v1y = static_cast<int64_t>(v1.y);
    int64_t v2x = static_cast<int64_t>(v2.x);
    int64_t v2y = static_cast<int64_t>(v2.y);
    int64_t res = v1x * v2y - v2x * v1y;
    if(res > 0)
        return 1;
    if(res < 0)
        return -1;
    return 0;
}

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

float glm::angleBetween(const glm::vec2& v1, const glm::vec2& v2)
{
    return glm::acos(glm::clamp(glm::dot(v1, v2) / (glm::length(v1) * glm::length(v2)), -1.0f, 1.0f));
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

float glm::hFOVtovFOV(float hFOV, float aspect)
{
    return glm::degrees(2.0f*glm::atan(1.0f/aspect * glm::tan(glm::radians(hFOV)*0.5f)));
}
