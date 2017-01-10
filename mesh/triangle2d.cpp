#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "mesh/mesh.h"
#include "io/saferead.h"

void CMesh::SEdge::NextFlapPosition()
{
    switch(m_flapPosition)
    {
        case FP_LEFT :
        m_flapPosition = FP_RIGHT; break;

        case FP_RIGHT :
        m_flapPosition = FP_BOTH; break;

        case FP_BOTH :
        m_flapPosition = FP_NONE; break;

        case FP_NONE :
        m_flapPosition = FP_LEFT; break;

    default : break;
    }
}

CMesh::STriangle2D* CMesh::SEdge::GetOtherTriangle(const STriangle2D *aFirstTri) const
{
    return (m_left == aFirstTri ?
            m_right :
            m_left);
}

void CMesh::SEdge::SetSnapped(bool snapped)
{
    m_snapped = snapped;
}

int CMesh::SEdge::GetOtherTriIndex(const STriangle2D *aFirstTri) const
{
    return (m_left == aFirstTri ?
            m_rightIndex :
            m_leftIndex);
}

CMesh::STriangle2D* CMesh::SEdge::GetAnyTriangle() const
{
    return (m_left == nullptr ?
            m_right :
            m_left);
}

int CMesh::SEdge::GetAnyTriIndex() const
{
    return (m_left == nullptr ?
            m_rightIndex :
            m_leftIndex);
}

CMesh::STriangle2D* CMesh::SEdge::GetTriangle(size_t index) const
{
    assert(index < 2);
    return (index == 0 ?
            m_left :
            m_right);
}

int CMesh::SEdge::GetTriIndex(size_t index) const
{
    assert(index < 2);
    return (index == 0 ?
            m_leftIndex :
            m_rightIndex);
}

bool CMesh::SEdge::IsSnapped() const
{
    return m_snapped;
}

CMesh::SEdge::EFlapPosition CMesh::SEdge::GetFlapPosition() const
{
    return m_flapPosition;
}

CMesh::SEdge::EFoldType CMesh::SEdge::GetFoldType() const
{
    return m_foldType;
}

void CMesh::STriangle2D::GroupHasTransformed(const glm::mat3 &parMx)
{
    using namespace glm;
    mat3 newMx = parMx * m_relativeMx;

    //glm::acos(1.000000012f) == nan!       x_x 5+ hours of debugging
    newMx[0][0] = clamp(newMx[0][0], -1.0f, 1.0f);
    newMx[1][0] = clamp(newMx[1][0], -1.0f, 1.0f);
    newMx[1][1] = clamp(newMx[1][1], -1.0f, 1.0f);
    newMx[0][1] = clamp(newMx[0][1], -1.0f, 1.0f);

    m_position = vec2(newMx[2][0], newMx[2][1]);
    mat2 rotMx;
    rotMx[0] = vec2(newMx[0][0], newMx[0][1]);
    rotMx[1] = vec2(newMx[1][0], newMx[1][1]);
    m_rotation = degrees(acos(newMx[0][0])*sign(asin(newMx[0][1])));
    for(int i=0; i<3; ++i)
    {
        m_normR[i] = rotMx*m_norm[i];
        m_vtxR[i] = rotMx*m_vtx[i];
        m_vtxRT[i] = m_vtxR[i]+m_position;
    }
}

void CMesh::STriangle2D::SetRelMx(glm::mat3 &invParentMx)
{
    m_relativeMx = invParentMx * GetMatrix();
}

glm::mat3 CMesh::STriangle2D::GetMatrix() const
{
    float rotRAD = glm::radians(m_rotation);
    glm::mat3 mx;
    mx[0] = glm::vec3(glm::cos(rotRAD), glm::sin(rotRAD), 0.0f);
    mx[1] = glm::vec3(-glm::sin(rotRAD), glm::cos(rotRAD), 0.0f);
    mx[2] = glm::vec3(m_position[0], m_position[1], 1.0f);
    return mx;
}

void CMesh::STriangle2D::Scale(float scale)
{
    for(int i=0; i<3; ++i)
    {
        m_vtx[i] *= scale;
        m_edgeLen[i] *= scale;
    }

    m_position *= scale;
    SetRotation(m_rotation);
}

void CMesh::STriangle2D::Init()
{
    for(int i=0; i<3; ++i)
    {
        m_vtxR[i] = m_vtxRT[i] = m_vtx[i];
        m_flapSharp[i] = false;
    }
    m_myGroup = nullptr;
    m_position = glm::vec2(0.0f, 0.0f);
    m_rotation = 0.0f;
    m_relativeMx = glm::mat3(1);
    m_edgeLen[0] = glm::distance(m_vtx[0], m_vtx[1]);
    m_edgeLen[1] = glm::distance(m_vtx[1], m_vtx[2]);
    m_edgeLen[2] = glm::distance(m_vtx[2], m_vtx[0]);
    ComputeNormals();
}

void CMesh::STriangle2D::SetRotation(float degCCW)
{
    m_rotation = degCCW;
    while(m_rotation >= 360.0f)
        m_rotation -= 360.0f;
    while(m_rotation < 0.0f)
        m_rotation += 360.0f;
    float rotRAD = glm::radians(m_rotation);
    glm::mat2 rotMx;
    rotMx[0] = glm::vec2(glm::cos(rotRAD), glm::sin(rotRAD));
    rotMx[1] = glm::vec2(-glm::sin(rotRAD), glm::cos(rotRAD));
    for(int i=0; i<3; ++i)
    {
        m_normR[i] = rotMx*m_norm[i];
        m_vtxR[i] = rotMx*m_vtx[i];
        m_vtxRT[i] = m_vtxR[i]+m_position;
    }
}

void CMesh::STriangle2D::SetPosition(glm::vec2 pos)
{
    m_position = pos;
    for(int i=0; i<3; ++i)
        m_vtxRT[i] = m_vtxR[i]+m_position;
}

bool CMesh::STriangle2D::Intersect(const STriangle2D &other) const
{
    for(int i=0; i<3; ++i)
    for(int j=0; j<3; ++j)
        if(EdgesIntersect(m_vtxRT[i], m_vtxRT[(i+1)%3], other.m_vtxRT[j], other.m_vtxRT[(j+1)%3]))
            return true;
    return false;
}

bool CMesh::STriangle2D::PointInside(const glm::vec2 &point) const
{
    for(int i=0; i<3; ++i)
    {
        glm::vec2 vB = point - m_vtxRT[i];
        glm::vec2 vA = m_vtxRT[(i+1)%3] - m_vtxRT[i];
        if(vA[0]*vB[1] - vB[0]*vA[1] < 0.0f)
            return false;
    }
    return true;
}

bool CMesh::STriangle2D::PointIsNearEdge(const glm::vec2 &point, const int &i, float &score) const
{
    float dv[2] = { glm::distance(point, m_vtxRT[i]),
                    glm::distance(point, m_vtxRT[(i+1)%3]) };

    if(m_edgeLen[i]*1.01f >= dv[0]+dv[1])
    {
        score = glm::abs(((m_vtxRT[(i+1)%3].x-m_vtxRT[i].x)*(point.y-m_vtxRT[i].y)-(m_vtxRT[(i+1)%3].y-m_vtxRT[i].y)*(point.x-m_vtxRT[i].x))/m_edgeLen[i]); //distance to edge
        return true;
    }
    return false;
}

float CMesh::STriangle2D::GetEdgeLen(size_t index) const
{
    assert(index < 3);
    return m_edgeLen[index];
}

const glm::vec2& CMesh::STriangle2D::operator[](size_t index) const
{
    assert(index < 3);
    return m_vtxRT[index];
}

CMesh::STriGroup* CMesh::STriangle2D::GetGroup() const
{
    return m_myGroup;
}

const int& CMesh::STriangle2D::ID() const
{
    return m_id;
}

bool CMesh::STriangle2D::IsFlapSharp(size_t index) const
{
    assert(index < 3);
    return m_flapSharp[index];
}

CMesh::SEdge* CMesh::STriangle2D::GetEdge(size_t index) const
{
    assert(index < 3);
    return m_edges[index];
}

const glm::vec2& CMesh::STriangle2D::GetNormal(size_t index) const
{
    assert(index < 3);
    return m_normR[index];
}

void CMesh::STriangle2D::ComputeNormals()
{
    for(int i=0; i<3; ++i)
    {
        glm::vec3 vFront = glm::vec3(m_vtx[(i+1)%3][0], m_vtx[(i+1)%3][1], 0.0f) - glm::vec3(m_vtx[i][0], m_vtx[i][1], 0.0f);
        glm::vec3 vRight = glm::normalize(glm::cross(vFront, glm::vec3(0.0f, 0.0f, 1.0f)));
        glm::vec2 nrm = glm::normalize(glm::vec2(vRight[0], vRight[1]));//*edgeLen[i]*0.1f;
        if(m_edgeLen[i] < 2.0f)
        {
            m_flapSharp[i] = true;
        }
        m_norm[i] = m_normR[i] = nrm;
    }
}

bool CMesh::STriangle2D::EdgesIntersect(const glm::vec2 &e1v1, const glm::vec2 &e1v2, const glm::vec2 &e2v1, const glm::vec2 &e2v2)
{
    float smallestEdgeLength = 0.01f*glm::min(glm::distance(e1v1, e1v2), glm::distance(e2v1, e2v2));
    if(glm::distance(e1v1, e2v1) < smallestEdgeLength ||
       glm::distance(e1v1, e2v2) < smallestEdgeLength ||
       glm::distance(e1v2, e2v1) < smallestEdgeLength ||
       glm::distance(e1v2, e2v2) < smallestEdgeLength)
        return false;
    glm::vec2 A = e1v2 - e1v1;
    glm::vec2 B1 = e2v1 - e1v1;
    glm::vec2 B2 = e2v2 - e1v1;
    bool b1 = (A[0]*B1[1] - B1[0]*A[1]) > 0.0f;
    bool b2 = (A[0]*B2[1] - B2[0]*A[1]) > 0.0f;
    A = e2v2 - e2v1;
    B1 = e1v1 - e2v1;
    B2 = e1v2 - e2v1;
    bool b3 = (A[0]*B1[1] - B1[0]*A[1]) > 0.0f;
    bool b4 = (A[0]*B2[1] - B2[0]*A[1]) > 0.0f;
    return (b1 || b2) && !(b1 && b2) &&
           (b3 || b4) && !(b3 && b4);
}

void CMesh::STriangle2D::Serialize(FILE *f) const
{
    std::fwrite(&m_id, sizeof(int), 1, f);
    std::fwrite(m_vtx, sizeof(glm::vec2), 3, f);
    std::fwrite(m_vtxR, sizeof(glm::vec2), 3, f);
    std::fwrite(m_vtxRT, sizeof(glm::vec2), 3, f);
    std::fwrite(m_norm, sizeof(glm::vec2), 3, f);
    std::fwrite(m_normR, sizeof(glm::vec2), 3, f);
    std::fwrite(m_flapSharp, sizeof(bool), 3, f);
    std::fwrite(m_edgeLen, sizeof(float), 3, f);
    std::fwrite(&m_position, sizeof(glm::vec2), 1, f);
    std::fwrite(&m_rotation, sizeof(float), 1, f);
    std::fwrite(m_angleOY, sizeof(float), 3, f);
    std::fwrite(&m_relativeMx, sizeof(glm::mat3), 1, f);
}

void CMesh::STriangle2D::Deserialize(FILE *f)
{
    SAFE_FREAD(&m_id, sizeof(int), 1, f);
    SAFE_FREAD(m_vtx, sizeof(glm::vec2), 3, f);
    SAFE_FREAD(m_vtxR, sizeof(glm::vec2), 3, f);
    SAFE_FREAD(m_vtxRT, sizeof(glm::vec2), 3, f);
    SAFE_FREAD(m_norm, sizeof(glm::vec2), 3, f);
    SAFE_FREAD(m_normR, sizeof(glm::vec2), 3, f);
    SAFE_FREAD(m_flapSharp, sizeof(bool), 3, f);
    SAFE_FREAD(m_edgeLen, sizeof(float), 3, f);
    SAFE_FREAD(&m_position, sizeof(glm::vec2), 1, f);
    SAFE_FREAD(&m_rotation, sizeof(float), 1, f);
    SAFE_FREAD(m_angleOY, sizeof(float), 3, f);
    SAFE_FREAD(&m_relativeMx, sizeof(glm::mat3), 1, f);
}

void CMesh::SEdge::Serialize(FILE *f) const
{
    std::fwrite(&m_leftIndex, sizeof(int), 1, f);
    std::fwrite(&m_rightIndex, sizeof(int), 1, f);
    std::fwrite(&m_angle, sizeof(float), 1, f);
    std::fwrite(&m_snapped, sizeof(bool), 1, f);
    std::fwrite(&m_flapPosition, sizeof(EFlapPosition), 1, f);
    std::fwrite(&m_foldType, sizeof(EFoldType), 1, f);
}

void CMesh::SEdge::Deserialize(FILE *f)
{
    SAFE_FREAD(&m_leftIndex, sizeof(int), 1, f);
    SAFE_FREAD(&m_rightIndex, sizeof(int), 1, f);
    SAFE_FREAD(&m_angle, sizeof(float), 1, f);
    SAFE_FREAD(&m_snapped, sizeof(bool), 1, f);
    SAFE_FREAD(&m_flapPosition, sizeof(EFlapPosition), 1, f);
    SAFE_FREAD(&m_foldType, sizeof(EFoldType), 1, f);
}
