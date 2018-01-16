#define GLM_ENABLE_EXPERIMENTAL
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
#include <glm/gtx/norm.hpp>
#include "geometric/aabbox.h"
#include "geometric/obbox.h"
#include "geometric/compgeom.h"

namespace
{

static const float mult = 1000.0f;

static int64_t len2(const glm::ivec2& vec)
{
    int64_t x = static_cast<int64_t>(vec.x);
    int64_t y = static_cast<int64_t>(vec.y);
    return x*x + y*y;
}

std::vector<glm::ivec2> GetStablePoints(const std::vector<glm::vec2>& points)
{
    std::vector<glm::ivec2> intPoints;
    intPoints.reserve(points.size());
    for(const glm::vec2& vec : points)
    {
        const glm::ivec2 vecInt(static_cast<int>(vec.x * mult),
                                static_cast<int>(vec.y * mult));
        if(std::find(intPoints.begin(), intPoints.end(), vecInt) == intPoints.end())
            intPoints.push_back(vecInt);
    }
    return intPoints;
}

std::vector<glm::vec2> GetConvexHull(const std::vector<glm::vec2>& inputPoints)
{
    //this is Graham Scan algorithm
    //first, move our points to integer numbers and remove duplicates
    std::vector<glm::ivec2> intPoints = GetStablePoints(inputPoints);

    const std::size_t sz = intPoints.size();
    if(sz < 3)
        throw std::logic_error("Trying to find convex hull for degenerate polygon!");

    //find a point definitely inside convex hull
    const glm::vec2 pivotF = (inputPoints[0] + inputPoints[1] + inputPoints[2]) / 3.0f;
    const glm::ivec2 pivot(static_cast<int>(pivotF.x * mult),
                           static_cast<int>(pivotF.y * mult));

    //sort points counter-clockwise around this point
    std::sort(intPoints.begin(), intPoints.end(), [&pivot](const glm::ivec2& v1, const glm::ivec2& v2)
    {
        const glm::ivec2 a = v1 - pivot;
        const glm::ivec2 b = v2 - pivot;
        if(a.y == 0 && a.x > 0)
            return true;
        if(b.y == 0 && b.x > 0)
            return false;
        if(a.y > 0 && b.y < 0)
            return true;
        if(a.y < 0 && b.y > 0)
             return false;
        int cross = glm::crossSign(a, b);
        if(cross > 0)
            return true;
        if(cross < 0)
            return false;
        return len2(a) < len2(b);
    });

    //find point that definitely belongs to convex hull
    std::size_t lowestIndex = 0;
    glm::ivec2 lowest = intPoints[0];
    for(std::size_t i = 1; i < intPoints.size(); i++)
    {
        if(intPoints[i].y < lowest.y || (intPoints[i].y == lowest.y && intPoints[i].x < lowest.x))
        {
            lowestIndex = i;
            lowest = intPoints[i];
        }
    }

    //create doubly-linked structure for Graham Scan algorithm
    struct GrahamNode
    {
        GrahamNode* next;
        GrahamNode* prev;
        glm::ivec2* pos;
    };
    std::vector<GrahamNode> nodes(sz);
    for(std::size_t i=0; i<sz; i++)
    {
        nodes[i].next = &nodes[(i+1)%sz];
        nodes[i].prev = &nodes[(i+sz-1)%sz];
        nodes[i].pos = &intPoints[i];
    }

    //perform actual Graham Scan with robust algorithm implementation
    //and not that bullshit from wikipedia and alike
    GrahamNode* start = &nodes[lowestIndex];
    GrahamNode* v = start;
    while(v->next != start)
    {
        int crossSign = glm::crossSign(*(v->next->pos) - *(v->pos), *(v->next->next->pos) - *(v->pos));
        if(crossSign > 0)
        {
            v = v->next;
        } else {
            v->next = v->next->next;
            v->next->prev = v;
            if(v != start)
                v = v->prev;
        }
    }

    //convex hull found! Now flush points to result vector and move them back to floats
    std::vector<glm::vec2> result;
    v = start;
    do
    {
        const glm::ivec2* ivec = v->pos;
        result.push_back(glm::vec2(static_cast<float>(ivec->x) / mult,
                                   static_cast<float>(ivec->y) / mult));
        v = v->next;
    } while(v != start);

    return result;
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

    BoxInfo minBox;
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

        if (minBoxPrice > price)
        {
            minBoxPrice = price;
            minBox.box = box;
            minBox.angle = angle;
        }
    }

    const glm::mat2 reverseRotMx = glm::rotation(-minBox.angle);
    std::vector<glm::vec2> obboxPoints = {minBox.box.GetLeftBottom(), minBox.box.GetRightBottom(),
                                          minBox.box.GetRightTop(),   minBox.box.GetLeftTop()};
    for(glm::vec2& p : obboxPoints)
        p = reverseRotMx * p;

    return SOBBox(obboxPoints[0], obboxPoints[1], obboxPoints[2], obboxPoints[3]);
}
