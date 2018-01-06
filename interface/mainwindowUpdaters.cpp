#include "ui_mainwindow.h"
#include "interface/mainwindow.h"
#include "interface/actionupdater.h"
#include "mesh/mesh.h"

namespace
{
class UMainWinUpdater : public CActionUpdater
{
public:
    UMainWinUpdater(const CMainWindow& wnd) : m_window(wnd) {}

protected:
    const CMainWindow& m_window;
};

class UHasModel : public UMainWinUpdater
{
public:
    UHasModel(const CMainWindow& wnd) : UMainWinUpdater(wnd)
    {
        Subscribe<CMainWindow::ModelStateChanged>(&CActionUpdater::RequestUpdate);
    }

    void Update(QAction* action) override
    {
        action->setEnabled(m_window.HasModel());
    }
};

class UUndo : public UMainWinUpdater
{
public:
    UUndo(const CMainWindow& wnd) : UMainWinUpdater(wnd)
    {
        Subscribe<CMainWindow::ModelStateChanged>(&CActionUpdater::RequestUpdate);
        Subscribe<CMesh::UndoRedoChanged>(&CActionUpdater::RequestUpdate);
    }

    void Update(QAction* action) override
    {
        action->setEnabled(m_window.HasModel() && m_window.GetModel()->CanUndo());
    }
};

class URedo : public UMainWinUpdater
{
public:
    URedo(const CMainWindow& wnd) : UMainWinUpdater(wnd)
    {
        Subscribe<CMainWindow::ModelStateChanged>(&CActionUpdater::RequestUpdate);
        Subscribe<CMesh::UndoRedoChanged>(&CActionUpdater::RequestUpdate);
    }

    void Update(QAction* action) override
    {
        action->setEnabled(m_window.HasModel() && m_window.GetModel()->CanRedo());
    }
};

template<typename TNotification>
class ULambda : public CActionUpdater
{
public:
    ULambda(std::function<void()>&& func) : m_func(std::move(func))
    {
        Subscribe<TNotification>(&CActionUpdater::RequestUpdate);
        SetActions({nullptr});
    }

    void Update(QAction* action) override
    {
        Q_UNUSED(action);
        m_func();
    }

private:
    std::function<void()> m_func;
};

}

void CMainWindow::RegisterUpdaters()
{
    auto updater = std::unique_ptr<CActionUpdater>(new UHasModel(*this));
    updater->SetActions({ui->actionPolypaint,
                         ui->actionAutoPack,
                         ui->actionLoad_Texture,
                         ui->actionCloseModel,
                         ui->actionScale,
                         ui->actionExport_Sheets,
                         ui->actionZoom_2D,
                         ui->actionZoom_3D,
                         ui->actionZoom_fit,
                         ui->actionModeFlaps,
                         ui->actionModeMove,
                         ui->actionModeRotate,
                         ui->actionModeSelect,
                         ui->actionModeSnap,
                         ui->actionSave,
                         ui->actionSave_As});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new UUndo(*this));
    updater->SetActions({ui->actionUndo});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new URedo(*this));
    updater->SetActions({ui->actionRedo});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new ULambda<CMesh::UndoRedoChanged>([this]()
    {
        if(HasModel())
            m_modelModified = true;
    }));
    m_actionUpdaters.emplace_back(std::move(updater));
}
