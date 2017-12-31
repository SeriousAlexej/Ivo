#ifndef OBBOX_H
#define OBBOX_H
#include <array>
#include <vector>
#include <glm/vec2.hpp>

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

SOBBox GetMinOBBox(const std::vector<glm::vec2>& points);

#endif // OBBOX_H
