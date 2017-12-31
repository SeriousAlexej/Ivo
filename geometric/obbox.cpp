#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "obbox.h"

SOBBox::SOBBox(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c, const glm::vec2 &d)
{
    points[0] = a;
    points[1] = b;
    points[2] = c;
    points[3] = d;
}

float SOBBox::GetArea() const
{
    return GetWidth() * GetHeight();
}

float SOBBox::GetWidth() const
{
    return glm::distance(points[0], points[1]);
}

float SOBBox::GetHeight() const
{
    return glm::distance(points[0], points[3]);
}

float SOBBox::GetRotation() const
{
    float angle = 0.0f;
    glm::vec2 norm = glm::normalize(points[1] - points[0]);
    angle = glm::degrees(glm::acos(glm::clamp(norm.x, -1.0f, 1.0f)));
    if(norm.y < 0.0f)
    {
        angle = -angle;
    }
    return angle;
}

const glm::vec2& SOBBox::operator[](std::size_t index) const
{
    return points.at(index);
}
