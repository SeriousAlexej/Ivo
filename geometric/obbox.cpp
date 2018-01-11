#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "geometric/compgeom.h"
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
    return glm::degrees(glm::angleFromTo(glm::vec2(1.0f, 0.0f), points[1] - points[0]));
}

const glm::vec2& SOBBox::operator[](std::size_t index) const
{
    return points.at(index);
}
