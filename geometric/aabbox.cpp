#include <algorithm>
#include "aabbox.h"

SAABBox2D::SAABBox2D(const glm::vec2 &rightBottom, const glm::vec2 &leftTop)
    : m_rightBottom(rightBottom), m_leftTop(leftTop)
{
}

SAABBox2D SAABBox2D::Union(const SAABBox2D &box1, const SAABBox2D &box2)
{
    SAABBox2D result = box1;
    result.m_leftTop.x = std::min(box1.m_leftTop.x, box2.m_leftTop.x);
    result.m_leftTop.y = std::max(box1.m_leftTop.y, box2.m_leftTop.y);
    result.m_rightBottom.x = std::max(box1.m_rightBottom.x, box2.m_rightBottom.x);
    result.m_rightBottom.y = std::min(box1.m_rightBottom.y, box2.m_rightBottom.y);
    return result;
}

bool SAABBox2D::Intersects(const SAABBox2D &box1, const SAABBox2D &box2)
{
    bool atLeft   = box1.m_rightBottom.x <= box2.m_leftTop.x;
    bool atRight  = box1.m_leftTop.x     >= box2.m_rightBottom.x;
    bool atTop    = box1.m_rightBottom.y >= box2.m_leftTop.y;
    bool atBottom = box1.m_leftTop.y     <= box2.m_rightBottom.y;

    return !(atLeft || atRight || atTop || atBottom);
}
