#ifndef MESH_H
#define MESH_H
#include <QUndoStack>
#include <string>
#include <vector>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <list>
#include <cstdio>
#include "modelLoader/structure.h"

#define IVO_VERSION 1

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

    inline const std::vector<glm::vec3>& GetNormals()   const { return m_flatNormals; }
    inline const std::vector<glm::vec2>& GetUVCoords()  const { return m_uvCoords; }
    inline const std::vector<glm::vec3>& GetVertices()  const { return m_vertices; }
    inline const std::vector<Triangle>&  GetTriangles() const { return m_triangles; }
    inline const glm::vec3*              GetAABBox()    const { return m_aabbox; }

    const CMesh::STriGroup*   GroupUnderCursor(glm::vec2 &curPos) const;
    void                      GetStuffUnderCursor(glm::vec2 &curPos, CMesh::STriangle2D*& tr, int &e) const;
    void                      Undo();
    void                      Redo();
    void                      NotifyGroupMovement(STriGroup& grp, const glm::vec2& oldPos);
    void                      NotifyGroupRotation(STriGroup& grp, float oldRot);
    void                      Serialize(FILE* f) const;
    void                      Deserialize(FILE* f);

private:
    void CalculateFlatNormals();
    void FillAdjTri_Gen2DTri();
    void DetermineFoldParams(int i, int j, int e1, int e2);
    void GroupTriangles(float maxAngleDeg);
    void PackGroups();
    void UpdateGroupDepth();
    void CalculateAABBox();

    std::vector<glm::vec2>      m_uvCoords;
    std::vector<glm::vec3>      m_normals;
    std::vector<glm::vec3>      m_vertices;
    std::vector<Triangle>       m_triangles;
    //generated stuff
    std::vector<glm::vec3>      m_flatNormals;
    std::vector<STriangle2D>    m_tri2D;
    std::list<SEdge>            m_edges;
    std::list<STriGroup>        m_groups;
    glm::vec3                   m_aabbox[8];

    QUndoStack                  m_undoStack;

    friend class CRenWin3D;
    friend class CRenWin2D;

public:
    struct STriangle2D
    {
        void                SetRelMx(glm::mat3 &invParentMx);
        glm::mat3           GetMatrix() const;
        void                SetRotation(float degCCW);
        void                SetPosition(glm::vec2 pos);
        bool                Intersect(const STriangle2D &other) const;
        bool                PointInside(const glm::vec2 &point) const;
        bool                PointIsNearEdge(const glm::vec2 &point, const int &e, float &score) const;

        const glm::vec2&    operator[](size_t index) const;

        const int&          ID() const;
        STriGroup*          GetGroup() const;
        bool                IsFlapSharp(size_t index) const;
        SEdge*              GetEdge(size_t index) const;
        const glm::vec2&    GetNormal(size_t index) const;

    private:
        void        Init();
        void        ComputeNormals();
        void        GroupHasTransformed(const glm::mat3 &parMx);
        static bool EdgesIntersect(const glm::vec2 &e1v1, const glm::vec2 &e1v2, const glm::vec2 &e2v1, const glm::vec2 &e2v2);

        int         m_id;
        glm::vec2   m_vtx[3]; //initial position (imagine this is constant)
        glm::vec2   m_vtxR[3]; //rotated only
        glm::vec2   m_vtxRT[3]; //rotated && translated
        glm::vec2   m_norm[3]; //perpendicular to edges
        glm::vec2   m_normR[3];
        bool        m_flapSharp[3];
        float       m_edgeLen[3];
        STriGroup*  m_myGroup = nullptr;
        glm::vec2   m_position;
        float       m_rotation;
        float       m_angleOY[3];
        glm::mat3   m_relativeMx;
        SEdge*      m_edges[3] = {nullptr, nullptr, nullptr};

        friend class CMesh;
    };

    struct SEdge
    {
        enum EFlapPosition
        {
            FP_LEFT=0,
            FP_RIGHT,
            FP_BOTH,
            FP_NONE
        };
        enum EFoldType
        {
            FT_MOUNTAIN,
            FT_VALLEY,
            FT_FLAT
        };

        void          SetSnapped(bool snapped);
        void          NextFlapPosition();
        inline bool   HasTwoTriangles() const { return m_left && m_right; }
        STriangle2D*  GetOtherTriangle(const STriangle2D* aFirstTri) const;
        int           GetOtherTriIndex(const STriangle2D* aFirstTri) const;
        STriangle2D*  GetAnyTriangle() const;
        int           GetAnyTriIndex() const;
        STriangle2D*  GetTriangle(size_t index) const;
        int           GetTriIndex(size_t index) const;
        bool          IsSnapped() const;
        EFlapPosition GetFlapPosition() const;
        EFoldType     GetFoldType() const;

    private:
        STriangle2D*    m_left = nullptr;
        STriangle2D*    m_right = nullptr;
        int             m_leftIndex;
        int             m_rightIndex;
        float           m_angle;
        bool            m_snapped;
        EFlapPosition   m_flapPosition;
        EFoldType       m_foldType;

        friend class CMesh;
    };

    struct STriGroup
    {
        STriGroup(CMesh *m);

        void                AttachGroup(STriangle2D* tr2, int e2);
        void                BreakGroup(STriangle2D* tr2, int e2);
        void                JoinEdge(STriangle2D* tr, int e);
        void                BreakEdge(STriangle2D* tr, int e);
        void                CentrateOrigin();
        bool                AddTriangle(STriangle2D* tr, STriangle2D* referal);
        void                RemTriangle(STriangle2D* tr);
        void                SetRotation(float angle);
        void                SetPosition(float x, float y);
        inline glm::vec2    GetPosition() const { return m_position; }
        inline float        GetRotation() const { return m_rotation; }
        const float&        GetDepth() const;
        const float&        GetAABBHalfSide() const;

        const std::list<STriangle2D*>& GetTriangles() const;

        static float        GetDepthStep();

    private:
        void                Serialize(FILE *f) const;
        void                Deserialize(FILE *f);

        std::list<STriangle2D*> m_tris;
        glm::vec2               m_toTopLeft;
        glm::vec2               m_toRightDown;
        float                   m_aabbHSide;
        float                   m_depth;
        glm::vec2               m_position;
        float                   m_rotation;
        glm::mat3               m_matrix;
        CMesh*                  m_msh = nullptr;

        static float            ms_depthStep;

        friend class CMesh;
    };
};

#endif // MESH_H
