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
#include <algorithm>
#include <cmath>
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
    width = std::abs(rightBottom.x - leftTop.x);
    height = std::abs(leftTop.y - rightBottom.y);
}

glm::vec2 SAABBox2D::GetRightTop() const
{
    return glm::vec2(GetRight(), GetTop());
}

glm::vec2 SAABBox2D::GetRightBottom() const
{
    return glm::vec2(GetRight(), GetBottom());
}

glm::vec2 SAABBox2D::GetLeftTop() const
{
    return glm::vec2(GetLeft(), GetTop());
}

glm::vec2 SAABBox2D::GetLeftBottom() const
{
    return glm::vec2(GetLeft(), GetBottom());
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

bool SAABBox2D::PointInside(const glm::vec2& point) const
{
    return point.x >= GetLeft()   && point.x <= GetRight() &&
           point.y >= GetBottom() && point.y <= GetTop();
}

SAABBox2D SAABBox2D::Union(const SAABBox2D& other) const
{
    float left   = std::min(GetLeft(),   other.GetLeft());
    float top    = std::max(GetTop(),    other.GetTop());
    float right  = std::max(GetRight(),  other.GetRight());
    float bottom = std::min(GetBottom(), other.GetBottom());
    SAABBox2D result;
    result.position = glm::vec2(right + left, top + bottom)*0.5f;
    result.width = std::abs(right - left);
    result.height = std::abs(top - bottom);
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
