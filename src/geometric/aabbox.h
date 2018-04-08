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
#ifndef AABBOX_H
#define AABBOX_H
#include <glm/vec2.hpp>

struct SAABBox2D
{
public:
    SAABBox2D();
    SAABBox2D(const glm::vec2 &rightBottom, const glm::vec2 &leftTop);

    bool PointInside(const glm::vec2& point) const;
    bool Intersects(const SAABBox2D& other) const;
    SAABBox2D Union(const SAABBox2D& other) const;

    glm::vec2 GetRightTop() const;
    glm::vec2 GetRightBottom() const;
    glm::vec2 GetLeftTop() const;
    glm::vec2 GetLeftBottom() const;
    float GetRight() const;
    float GetLeft() const;
    float GetTop() const;
    float GetBottom() const;

    float width;
    float height;
    glm::vec2 position;
};

#endif
