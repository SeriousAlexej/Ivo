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
