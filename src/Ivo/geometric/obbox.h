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
#ifndef OBBOX_H
#define OBBOX_H
#include <array>
#include <vector>
#include <functional>
#include <glm/vec2.hpp>

struct SAABBox2D;

struct SOBBox
{
public:
    SOBBox() = default;
    SOBBox(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& d);
    SOBBox(const SOBBox&) = default;
    ~SOBBox() = default;
    SOBBox& operator=(const SOBBox&) = default;

    const glm::vec2& operator[](std::size_t index) const;

    const std::array<glm::vec2, 4>& GetPoints() const { return points; }

    float GetArea() const;
    float GetRotation() const;
    float GetWidth() const;
    float GetHeight() const;

private:
    std::array<glm::vec2, 4> points;
};

SOBBox GetMinOBBox(const std::vector<glm::vec2>& points, std::function<float(const SAABBox2D&)> criteria = nullptr);

#endif // OBBOX_H
