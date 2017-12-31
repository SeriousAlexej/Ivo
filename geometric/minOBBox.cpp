#include <cmath>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <set>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "geometric/obbox.h"

namespace
{
struct GrahamPointCmp
{
    GrahamPointCmp(const glm::vec2& lowest)
    {
        this->lowest = lowest;
    }

    bool operator()(const glm::vec2& a, const glm::vec2& b) const
    {
        float thetaA = glm::atan(a.y - lowest.y, a.x - lowest.x);
        float thetaB = glm::atan(b.y - lowest.y, b.x - lowest.x);

        if(thetaA < thetaB)
        {
            return true;
        } else if(thetaA > thetaB) {
            return false;
        } else {
            float distanceA = glm::distance(lowest, a);
            float distanceB = glm::distance(lowest, b);

            if(distanceA < distanceB)
                return true;
            else
                return false;
        }
    }

private:
    glm::vec2 lowest;
};

enum Turn
{
    CLOCKWISE,
    COUNTER_CLOCKWISE,
    COLLINEAR
};

Turn GetTurn(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    float crossProduct = ((b.x - a.x) * (c.y - a.y)) -
                          ((b.y - a.y) * (c.x - a.x));

    if(crossProduct > 0.0f)
        return COUNTER_CLOCKWISE;
    else if(crossProduct < 0.0f)
        return CLOCKWISE;
    else
        return COLLINEAR;
}

bool AreAllCollinear(const std::vector<glm::vec2>& points)
{
    if(points.size() < 2)
    {
        return true;
    }

    const glm::vec2& a = points[0];
    const glm::vec2& b = points[1];

    for(int i = 2, sz = points.size(); i < sz; i++)
    {
        const glm::vec2& c = points[i];

        if(GetTurn(a, b, c) != COLLINEAR)
            return false;
    }

    return true;
}

const glm::vec2& GetLowestPoint(const std::vector<glm::vec2>& points)
{
    const glm::vec2* lowest = &points[0];

    for(int i = 1, sz = points.size(); i < sz; i++)
    {
        const glm::vec2* temp = &points[i];

        if(temp->y < lowest->y || (temp->y == lowest->y && temp->x < lowest->x))
            lowest = temp;
    }

    return *lowest;
}

std::vector<glm::vec2> GetSortedPointSet(const std::vector<glm::vec2>& points)
{
    std::set<glm::vec2, GrahamPointCmp> pointsSorted(GrahamPointCmp(GetLowestPoint(points)));
    pointsSorted.insert(points.begin(), points.end());
    return std::vector<glm::vec2>(pointsSorted.begin(), pointsSorted.end());
}

std::vector<glm::vec2> GetConvexHull(const std::vector<glm::vec2>& points)
{
    std::vector<glm::vec2> sorted = GetSortedPointSet(points);

    if(sorted.size() < 3 || AreAllCollinear(sorted))
        throw std::logic_error("Trying to find convex hull for degenerate polygon!");

    std::deque<glm::vec2> stack;
    stack.push_back(sorted[0]);
    stack.push_back(sorted[1]);
    stack.push_back(sorted[2]);

    for(int i = 3, sz = sorted.size(); i < sz; i++)
    {
        while (stack.size() >= 2 && GetTurn(stack[stack.size()-2], stack.back(), sorted[i]) != COUNTER_CLOCKWISE)
           stack.pop_back();
        stack.push_back(sorted[i]);
    }

    stack.push_back(sorted[0]);

    return {stack.begin(), stack.end()};
}

} //namespace anonymous

SOBBox GetMinOBBox(const std::vector<glm::vec2>& points)
{
    struct Rect2D
    {
        Rect2D()
        {
        }

        Rect2D(const glm::vec2& a, const glm::vec2& c)
        {
            location = a;
            size = c - a;
        }

        float Area() const
        {
            return size.x * size.y;
        }

        std::vector<glm::vec2> Points()
        {
            return
            {
                glm::vec2(location.x, location.y),
                glm::vec2(location.x + size.x, location.y),
                glm::vec2(location.x + size.x, location.y + size.y),
                glm::vec2(location.x, location.y + size.y)
            };
        }

        glm::vec2 location;
        glm::vec2 size;
    };

    struct Segment2D
    {
        Segment2D(const glm::vec2& a, const glm::vec2& b)
        {
            A = a;
            B = b;
        }

        glm::vec2 A;
        glm::vec2 B;
    };

    auto AngleToXAxis = [](const Segment2D& s) -> float
    {
        glm::vec2 delta = s.A - s.B;
        return -glm::atan(delta.y / delta.x);
    };

    auto Rotate = [](const glm::vec2& v, float angle) -> glm::vec2
    {
        float x = v.x*glm::cos(angle) - v.y*glm::sin(angle);
        float y = v.x*glm::sin(angle) + v.y*glm::cos(angle);
        return glm::vec2(x, y);
    };

    std::vector<glm::vec2> hullPoints = GetConvexHull(points);

    Rect2D minBox;
    float minBoxArea = std::numeric_limits<float>::max();
    float minAngle = 0.0f;

    for (int i = 0, sz = hullPoints.size(); i < sz; i++)
    {
        int nextIndex = i + 1;

        const glm::vec2& current = hullPoints[i];
        const glm::vec2& next = hullPoints[nextIndex % hullPoints.size()];

        Segment2D segment(current, next);

        float top    = std::numeric_limits<float>::lowest();
        float bottom = std::numeric_limits<float>::max();
        float left   = std::numeric_limits<float>::max();
        float right  = std::numeric_limits<float>::lowest();

        float angle = AngleToXAxis(segment);

        for (glm::vec2& p : hullPoints)
        {
            glm::vec2 rotatedPoint = Rotate(p, angle);

            top = glm::max(top, rotatedPoint.y);
            bottom = glm::min(bottom, rotatedPoint.y);
            left = glm::min(left, rotatedPoint.x);
            right = glm::max(right, rotatedPoint.x);
        }

        Rect2D box(glm::vec2(left, bottom), glm::vec2(right, top));

        if (minBoxArea > box.Area())
        {
            minBoxArea = box.Area();
            minBox = box;
            minAngle = angle;
        }
    }

    std::vector<glm::vec2> obboxPoints = minBox.Points();
    for(glm::vec2& p : obboxPoints)
        p = Rotate(p, -minAngle);

    return SOBBox(obboxPoints[0], obboxPoints[1], obboxPoints[2], obboxPoints[3]);
}
