#include <cmath>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <set>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "geometric/aabbox.h"
#include "geometric/obbox.h"
#include "geometric/compgeom.h"
#include "settings/settings.h"

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
    const float crossProduct = glm::cross(b - a, c - a);

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
        return true;

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

SOBBox GetMinOBBox(const std::vector<glm::vec2>& points, std::function<float(const SAABBox2D&)> criteria)
{
    if(!criteria)
        criteria = [](const SAABBox2D& b) -> float { return b.width * b.height; };

    std::vector<glm::vec2> hullPoints = GetConvexHull(points);

    struct BoxInfo
    {
        SAABBox2D box;
        float angle;
    };

    std::vector<BoxInfo> minBoxes;
    float minBoxPrice = std::numeric_limits<float>::max();

    for (int i = 0, sz = hullPoints.size(); i < sz; i++)
    {
        const glm::vec2& curr = hullPoints[i];
        const glm::vec2& next = hullPoints[(i+1)%sz];

        float top    = std::numeric_limits<float>::lowest();
        float bottom = std::numeric_limits<float>::max();
        float left   = std::numeric_limits<float>::max();
        float right  = std::numeric_limits<float>::lowest();

        const float angle = glm::angleFromTo(next - curr, glm::vec2(1.0f, 0.0f));
        const glm::mat2 rotMx = glm::rotation(angle);

        for (glm::vec2& p : hullPoints)
        {
            const glm::vec2 rotatedPoint = rotMx * p;

            top    = glm::max(top,    rotatedPoint.y);
            bottom = glm::min(bottom, rotatedPoint.y);
            left   = glm::min(left,   rotatedPoint.x);
            right  = glm::max(right,  rotatedPoint.x);
        }

        const SAABBox2D box(glm::vec2(right, bottom), glm::vec2(left, top));
        const float price = criteria(box);

        if (minBoxPrice >= price)
        {
            if(minBoxPrice != price)
            {
                minBoxes.clear();
                minBoxPrice = price;
            }
            BoxInfo info;
            info.box = box;
            info.angle = angle;
            minBoxes.push_back(info);
        }
    }

    BoxInfo* minBoxInfo = &minBoxes[0];
    minBoxPrice = minBoxInfo->box.width;
    for(auto& info : minBoxes)
    {
        const float price = info.box.width;
        if(minBoxPrice > price)
        {
            minBoxInfo = &info;
            minBoxPrice = price;
        }
    }
    const SAABBox2D& minBox = minBoxInfo->box;
    const glm::mat2 reverseRotMx = glm::rotation(-minBoxInfo->angle);
    std::vector<glm::vec2> obboxPoints = {minBox.GetLeftBottom(), minBox.GetRightBottom(),
                                          minBox.GetRightTop(),   minBox.GetLeftTop()};
    for(glm::vec2& p : obboxPoints)
        p = reverseRotMx * p;

    return SOBBox(obboxPoints[0], obboxPoints[1], obboxPoints[2], obboxPoints[3]);
}
