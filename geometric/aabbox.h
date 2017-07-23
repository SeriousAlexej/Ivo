#ifndef AABBOX_H
#define AABBOX_H
#include <glm/vec2.hpp>

struct SAABBox2D
{
public:
    SAABBox2D(const glm::vec2& rightBottom, const glm::vec2& leftTop);
    ~SAABBox2D() = default;

    static SAABBox2D Union(const SAABBox2D& box1, const SAABBox2D& box2);
    static bool      Intersects(const SAABBox2D& box1, const SAABBox2D& box2);

    glm::vec2 m_rightBottom;
    glm::vec2 m_leftTop;
};

#endif
