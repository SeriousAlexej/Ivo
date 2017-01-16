#ifndef IVO_COMMAND_H
#define IVO_COMMAND_H
#include <QUndoCommand>
#include <glm/vec2.hpp>
#include <list>

enum ECommandType
{
    CT_ROTATE,
    CT_MOVE,
    CT_JOIN_GROUPS,
    CT_BREAK_GROUP,
    CT_SNAP_EDGE,
    CT_BREAK_EDGE,
    CT_SCALE
};

class CAtomicCommand
{
public:
    explicit CAtomicCommand(ECommandType actionType);

    void SetTriangle(void* tr);
    void SetEdge(int e);
    void SetTranslation(const glm::vec2& trans);
    void SetRotation(float rot);
    void SetScale(float sca);

    void Redo() const;
    void Undo() const;

private:
    glm::vec2    m_translation;
    float        m_rotation;
    float        m_scale;
    void*        m_triangle;
    int          m_edge;
    ECommandType m_type;
};

class CIvoCommand : public QUndoCommand
{
public:
    void AddAction(const CAtomicCommand& action);
    void AddAction(CIvoCommand&& cmd);

    virtual void undo() override;
    virtual void redo() override;

private:
    std::list<CAtomicCommand> m_actions;
};

#endif // IVO_COMMAND_H
