#include <algorithm>
#include <glm/vec2.hpp>
#include "aabbox.h"

SAABBox2D::SAABBox2D()
    : width(0.0f)
    , height(0.0f)
    , position(0.0f, 0.0f)
{
}

SAABBox2D::SAABBox2D(const glm::vec2 &rightBottom, const glm::vec2 &leftTop)
{
    position = glm::vec2(rightBottom.x + leftTop.x, leftTop.y + rightBottom.y)*0.5f;
    width = std::fabs(rightBottom.x - leftTop.x);
    height = std::fabs(leftTop.y - rightBottom.y);
}

glm::vec2 SAABBox2D::GetRightBottom() const
{
    return glm::vec2(GetRight(), GetBottom());
}

glm::vec2 SAABBox2D::GetLeftTop() const
{
    return glm::vec2(GetLeft(), GetTop());
}

float SAABBox2D::GetBottom() const
{
    return position.y - height * 0.5f;
}

float SAABBox2D::GetTop() const
{
    return position.y + height * 0.5f;
}

float SAABBox2D::GetLeft() const
{
    return position.x - width * 0.5f;
}

float SAABBox2D::GetRight() const
{
    return position.x + width * 0.5f;
}

SAABBox2D SAABBox2D::Union(const SAABBox2D& other) const
{
    float left   = std::min(GetLeft(),   other.GetLeft());
    float top    = std::max(GetTop(),    other.GetTop());
    float right  = std::max(GetRight(),  other.GetRight());
    float bottom = std::min(GetBottom(), other.GetBottom());
    SAABBox2D result;
    result.position = glm::vec2(right + left, top + bottom)*0.5f;
    result.width = std::fabs(right - left);
    result.height = std::fabs(top - bottom);
    return result;
}

bool SAABBox2D::Intersects(const SAABBox2D& other) const
{
    bool atLeft   = GetRight()  <= other.GetLeft();
    bool atRight  = GetLeft()   >= other.GetRight();
    bool atTop    = GetBottom() >= other.GetTop();
    bool atBottom = GetTop()    <= other.GetBottom();

    return !(atLeft || atRight || atTop || atBottom);
}
