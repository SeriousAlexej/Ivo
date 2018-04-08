/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MESH_H
#define MESH_H
#include <QUndoStack>
#include <QJsonObject>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <list>
#include <cstddef>
#include "pdo/pdotools.h"
#include "geometric/aabbox.h"
#include "notification/notification.h"

extern const int IVO_VERSION;

class CIvoCommand;
struct aiScene;
struct aiNode;

class CMesh
{
public:
    NOTIFICATION(UndoRedoChanged);
    NOTIFICATION(GroupStructureChanging);

public:
    struct SEdge;
    struct STriangle2D;
    struct STriGroup;

    CMesh();
    ~CMesh();

    void                        LoadMesh(const std::string& path);
    void                        LoadFromPDO(const std::vector<PDO_Face>&                  faces,
                                            const std::vector<std::unique_ptr<PDO_Edge>>& edges,
                                            const std::vector<glm::vec3>&                 vertices3D,
                                            const std::unordered_map<unsigned, PDO_Part>& parts);

    float                       GetBSphereRadius() const { return m_bSphereRadius; }
    const glm::vec3*            GetAABBox()        const { return m_aabbox; }
    const std::vector
        <glm::vec3>&            GetNormals()       const { return m_flatNormals; }
    const std::vector
        <glm::vec2>&            GetUVCoords()      const { return m_uvCoords; }
    const std::vector
        <glm::vec3>&            GetVertices()      const { return m_vertices; }
    const std::vector
        <glm::uvec4>&           GetTriangles()     const { return m_triangles; }
    const std::list
        <SEdge>&                GetEdges()         const { return m_edges; }
    const std::list
        <STriGroup>&            GetGroups()        const { return m_groups; }
    const std::unordered_set
        <std::size_t>&          GetPickedTris()    const { return m_pickTriIndices; }

    const std::unordered_map
        <unsigned,std::string>& GetMaterials()     const { return m_materials; }

    void                        SetMaterials(const std::unordered_map<unsigned, std::string>& materials) { m_materials = materials; }

    std::vector<STriGroup*>     GetGroupsInRange(const SAABBox2D& range);
    CMesh::STriGroup*           GroupUnderCursor(const glm::vec2& curPos);
    void                        GetStuffUnderCursor(const glm::vec2& curPos, CMesh::STriangle2D*& tr, int &e) const;
    bool                        CanUndo() const;
    bool                        CanRedo() const;
    void                        Undo();
    void                        Redo();
    void                        Clear();
    void                        NotifyGroupsMovement(const std::vector<STriGroup*>& groups, const std::vector<glm::vec2>& oldPositions);
    void                        NotifyGroupsRotation(const std::vector<STriGroup*>& groups, const std::vector<float>& oldRotations);
    void                        NotifyGroupsTransformation(const std::vector<STriGroup*>& groups, const std::vector<glm::vec2>& oldPositions, const std::vector<float>& oldRotations);
    QJsonObject                 Serialize() const;
    void                        Deserialize(const QJsonObject& obj);
    void                        Scale(const float scale);
    bool                        PackGroups(bool undoable=true);
    glm::vec3                   GetSizeMillimeters() const;
    glm::vec3                   GetAABBoxCenter() const;
    void                        SetTriangleAsPicked(std::size_t index);
    void                        SetTriangleAsUnpicked(std::size_t index);
    bool                        IsTrianglePicked(std::size_t index) const;
    void                        ClearPickedTriangles();
    void                        GroupPickedTriangles();
    SAABBox2D                   GetAABBox2D() const;
    bool                        Intersects(const SAABBox2D& bbox) const;

private:
    static CMesh*               GetMesh() { return g_Mesh; }
    void                        ApplyScale(const float scale);
    void                        AddMeshesFromAIScene(const aiScene* scene, const aiNode* node);
    void                        CalculateFlatNormals();
    void                        FillAdjTri_Gen2DTri();
    void                        DetermineFoldParams(std::size_t i, std::size_t j, int e1, int e2);
    void                        GroupTriangles(float maxAngleDeg);
    void                        UpdateGroupDepth();
    void                        CalculateAABBox();
    void                        SetFoldType(SEdge& edg);

    static CMesh*               g_Mesh;
    std::vector<glm::vec2>      m_uvCoords;
    std::vector<glm::vec3>      m_normals;
    std::vector<glm::vec3>      m_vertices;
    std::vector<glm::uvec4>     m_triangles; //vtx1 index, vtx2 index, vtx3 index, mtl index
    std::unordered_set
        <std::size_t>           m_pickTriIndices;
    std::unordered_map
        <unsigned, std::string> m_materials;
    //generated stuff
    std::vector<glm::vec3>      m_flatNormals;
    std::vector<STriangle2D>    m_tri2D;
    std::list<SEdge>            m_edges;
    std::list<STriGroup>        m_groups;
    glm::vec3                   m_aabbox[8];
    float                       m_bSphereRadius;

    QUndoStack                  m_undoStack;

    friend class CAtomicCommand;

public:
    struct STriangle2D
    {
        STriangle2D() = default;

        glm::mat3               GetMatrix() const;
        bool                    Intersect(const STriangle2D &other) const;
        bool                    PointInside(const glm::vec2 &point) const;
        bool                    PointIsNearEdge(const glm::vec2 &point, const int &e, float &score) const;

        const glm::vec2&        operator[](size_t index) const;

        const std::size_t&      ID() const;
        STriGroup*              GetGroup() const;
        bool                    IsFlapSharp(size_t index) const;
        SEdge*                  GetEdge(size_t index) const;
        const glm::vec2&        GetNormal(size_t index) const;
        float                   GetEdgeLen(size_t index) const;

    private:
        void                    Init();
        void                    SetRelMx(glm::mat3 &invParentMx);
        void                    SetRotation(float degCCW);
        void                    SetPosition(glm::vec2 pos);
        void                    Scale(const float scale);
        void                    ComputeNormals();
        void                    GroupHasTransformed(const glm::mat3 &parMx);
        static bool             EdgesIntersect(const glm::vec2 &e1v1, const glm::vec2 &e1v2, const glm::vec2 &e2v1, const glm::vec2 &e2v2);

        QJsonObject             Serialize() const;
        void                    Deserialize(const QJsonObject& obj);

        std::size_t             m_id;
        glm::vec2               m_vtx[3]; //initial position (imagine this is constant)
        glm::vec2               m_vtxR[3]; //rotated only
        glm::vec2               m_vtxRT[3]; //rotated && translated
        glm::vec2               m_norm[3]; //perpendicular to edges
        glm::vec2               m_normR[3];
        bool                    m_flapSharp[3];
        float                   m_edgeLen[3];
        STriGroup*              m_myGroup = nullptr;
        glm::vec2               m_position;
        float                   m_rotation;
        float                   m_angleOY[3];
        glm::mat3               m_relativeMx;
        SEdge*                  m_edges[3] = {nullptr, nullptr, nullptr};

        friend class CMesh;
        friend struct CMesh::STriGroup;
    };

    struct SEdge
    {
        SEdge() = default;
        //non-copyable
        SEdge(const SEdge& o) = delete;
        SEdge(const SEdge&& o) = delete;
        SEdge& operator=(const SEdge& o) = delete;
        SEdge& operator=(const SEdge&& o) = delete;

        enum EFlapPosition
        {
            FP_LEFT=1,
            FP_RIGHT=2,
            FP_BOTH=3,
            FP_NONE=0
        };
        enum EFoldType
        {
            FT_MOUNTAIN,
            FT_VALLEY,
            FT_FLAT
        };

        float                   GetAngle() const { return m_angle; }
        void                    SetSnapped(bool snapped);
        void                    NextFlapPosition();
        inline bool             HasTwoTriangles() const { return m_left && m_right; }
        STriangle2D*            GetOtherTriangle(const STriangle2D* aFirstTri) const;
        int                     GetOtherTriIndex(const STriangle2D* aFirstTri) const;
        STriangle2D*            GetAnyTriangle() const;
        int                     GetAnyTriIndex() const;
        STriangle2D*            GetTriangle(size_t index) const;
        int                     GetTriIndex(size_t index) const;
        bool                    IsSnapped() const;
        EFlapPosition           GetFlapPosition() const;
        EFoldType               GetFoldType() const;

    private:
        QJsonObject             Serialize() const;
        void                    Deserialize(const QJsonObject& obj);

        STriangle2D*            m_left = nullptr;
        STriangle2D*            m_right = nullptr;
        int                     m_leftIndex;
        int                     m_rightIndex;
        float                   m_angle;
        bool                    m_snapped;
        EFlapPosition           m_flapPosition;
        EFoldType               m_foldType;

        friend class CMesh;
    };

    struct STriGroup
    {
        STriGroup();
        //non-copyable
        STriGroup(const STriGroup& o) = delete;
        STriGroup(const STriGroup&& o) = delete;
        STriGroup& operator=(const STriGroup& o) = delete;
        STriGroup& operator=(const STriGroup&& o) = delete;

        SAABBox2D               GetAABBox() const;
        void                    JoinEdge(STriangle2D* tr, int e);
        void                    BreakEdge(STriangle2D* tr, int e);
        void                    SetRotation(float angle);
        void                    SetPosition(float x, float y);
        inline glm::vec2        GetPosition() const { return m_position; }
        inline float            GetRotation() const { return m_rotation; }
        const float&            GetDepth() const;
        const float&            GetAABBHalfSide() const;

        const std::list
            <STriangle2D*>&     GetTriangles() const;

        static float            GetDepthStep();

    private:
        void                    CentrateOrigin();
        bool                    AddTriangle(STriangle2D* tr, STriangle2D* referal);
        void                    AttachGroup(STriangle2D* tr2, int e2);
        void                    BreakGroup(STriangle2D* tr2, int e2);
        CIvoCommand*            GetJoinEdgeCmd(STriangle2D* tr, int e);
        CIvoCommand*            GetBreakEdgeCmd(STriangle2D* tr, int e);
        QJsonObject             Serialize() const;
        void                    Deserialize(const QJsonObject& obj);
        void                    Scale(const float scale);
        void                    ResetBBoxVectors();
        void                    RecalcBBoxVectors();

        std::list<STriangle2D*> m_tris;
        glm::vec2               m_toTopLeft;
        glm::vec2               m_toRightDown;
        float                   m_aabbHSide;
        float                   m_depth;
        glm::vec2               m_position;
        float                   m_rotation;
        glm::mat3               m_matrix;

        static float            ms_depthStep;

        friend class CMesh;
        friend class CAtomicCommand;
    };
};

#endif // MESH_H
