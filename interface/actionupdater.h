#ifndef ACTION_UPDATER_H
#define ACTION_UPDATER_H
#include <vector>
#include <utility>
#include <QAction>
#include "notification/subscriber.h"

class CActionUpdater : public Subscriber
{
public:
    CActionUpdater() = default;
    virtual ~CActionUpdater() = default;

    virtual void Update(QAction* action) = 0;

    void RequestUpdate()
    {
        for(auto action : m_actions)
            Update(action);
    }

    void SetActions(std::vector<QAction*>&& actions)
    {
        m_actions = std::move(actions);
    }

private:
    std::vector<QAction*> m_actions;
};

#endif
