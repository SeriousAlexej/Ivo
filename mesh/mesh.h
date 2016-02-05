#ifndef MESH_H
#define MESH_H
#include <string>
#include <vector>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <list>
#include "modelLoader/structure.h"

class CMesh
{
public:
    struct SEdge;
    struct STriangle2D;
    struct STriGroup;

    CMesh();
    ~CMesh();
    bool LoadMesh(std::string path);
    void Clear();

    inline const std::vector<glm::vec3> &GetNormals()   const { return m_flatNormals; }
    inline const std::vector<glm::vec2> &GetUVCoords()  const { return m_uvCoords; }
    inline const std::vector<glm::vec3> &GetVertices()  const { return m_vertices; }
    inline const std::vector<Triangle>  &GetTriangles() const { return m_triangles; }

    const CMesh::STriGroup*   GroupUnderCursor(glm::vec2 &curPos) const;
    void                      GetStuffUnderCursor(glm::vec2 &curPos, CMesh::STriangle2D*& tr, int &e) const;

private:
    void CalculateFlatNormals();
    void FillAdjTri_Gen2DTri();
    void DetermineFoldParams(int i, int j, int e1, int e2);
    void GroupTriangles(float maxAngleDeg);
    void PackGroups();
    void UpdateGroupDepth();

private:
    std::vector<glm::vec2>      m_uvCoords;
    std::vector<glm::vec3>      m_normals;
    std::vector<glm::vec3>      m_vertices;
    std::vector<Triangle>       m_triangles;
    //generated stuff
    std::vector<glm::vec3>      m_flatNormals;
    std::vector<STriangle2D>    m_tri2D;
    std::list<SEdge>            m_edges;
    std::list<STriGroup>        m_groups;

    friend class CRenWin3D;
    friend class CRenWin2D;

public:
    struct STriangle2D
    {
        void        GroupHasTransformed(glm::mat3 &parMx);
        void        SetRelMx(glm::mat3 &invParentMx);
        glm::mat3   GetMatrix() const;
        void        Init();
        void        SetRotation(float degCCW);
        void        SetPosition(glm::vec2 pos);
        bool        Intersect(const STriangle2D &other) const;
        bool        PointInside(const glm::vec2 &point) const;
        bool        PointIsNearEdge(const glm::vec2 &point, const int &e, float &score) const;

        int         m_id;
        glm::vec2   m_vtx[3]; //initial position (imagine this is constant)
        glm::vec2   m_vtxR[3]; //rotated only
        glm::vec2   m_vtxRT[3]; //rotated && translated
        glm::vec2   m_norm[3]; //perpendicular to edges
        glm::vec2   m_normR[3];
        bool        m_flapSharp[3];
        float       m_edgeLen[3];
        STriGroup*  m_myGroup;
        glm::vec2   m_position;
        float       m_rotation;
        float       m_angleOY[3];
        glm::mat3   m_relativeMx;
        SEdge*      m_edges[3];

    private:
        void        ComputeNormals();
        static bool EdgesIntersect(const glm::vec2 &e1v1, const glm::vec2 &e1v2, const glm::vec2 &e2v1, const glm::vec2 &e2v2);
    };

    struct SEdge
    {
        void        NextFlapPosition();
        inline bool HasTwoTriangles() const { return m_left && m_right; }

        STriangle2D*    m_left;
        STriangle2D*    m_right;
        int             m_leftIndex;
        int             m_rightIndex;
        float           m_angle;
        bool            m_snapped;
        enum
        { FP_LEFT=0,
          FP_RIGHT,
          FP_BOTH,
          FP_NONE }     m_flapPosition;
        enum
        { FT_MOUNTAIN,
          FT_VALLEY,
          FT_FLAT }     m_foldType;

    };

    struct STriGroup
    {
    public:
        STriGroup(CMesh *m);

        void                JoinEdge(STriangle2D* tr, int e);
        void                BreakEdge(STriangle2D* tr, int e);
        void                CentrateOrigin();
        bool                AddTriangle(STriangle2D* tr, STriangle2D* referal);
        void                RemTriangle(STriangle2D* tr);
        void                SetRotation(float angle);
        void                SetPosition(float x, float y);
        inline glm::vec2    GetPosition() const { return m_position; }
        inline float        GetRotation() const { return m_rotation; }

        std::list<STriangle2D*> m_tris;
        glm::vec2               m_toTopLeft;
        glm::vec2               m_toRightDown;
        float                   m_aabbHSide;
        float                   m_depth;
        static float            ms_depthStep;

    private:
        glm::vec2   m_position;
        float       m_rotation;
        glm::mat3   m_matrix;
        CMesh*      m_msh;

        friend class CMesh;
    };
};

#endif // MESH_H
