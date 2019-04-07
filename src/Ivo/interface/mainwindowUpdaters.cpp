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
#include "ui_mainwindow.h"
#include "interface/mainwindow.h"
#include "interface/actionupdater.h"
#include "settings/settings.h"
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

class USetting : public CActionUpdater
{
public:
    USetting(std::function<bool()>&& checker) : m_checker(std::move(checker))
    {
        Subscribe<CSettings::Changed>(&CActionUpdater::RequestUpdate);
    }

    void Update(QAction* action) override
    {
        action->setChecked(m_checker());
    }

private:
    std::function<bool()> m_checker;
};

template<typename TNotification>
class ULambda : public CActionUpdater
{
public:
    ULambda(std::function<void()>&& func)
    {
        Subscribe<TNotification>(std::move(func));
    }

    void Update(QAction* action) override { Q_UNUSED(action); }
};

}

void CMainWindow::RegisterUpdaters()
{
    const CSettings& sett = CSettings::GetInstance();

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

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_EDGES) != 0; }));
    updater->SetActions({ui->actionShow_Edges});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_FLAPS) != 0; }));
    updater->SetActions({ui->actionShow_Flaps});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_FOLDS) != 0; }));
    updater->SetActions({ui->actionShow_Folds});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_TEXTR) != 0; }));
    updater->SetActions({ui->actionShow_Texture});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_LIGHT) != 0; }));
    updater->SetActions({ui->actionToggle_Lighting});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new USetting([&sett]{ return (sett.GetRenderFlags() & CSettings::R_GRID) != 0; }));
    updater->SetActions({ui->actionShow_Grid});
    m_actionUpdaters.emplace_back(std::move(updater));

    updater = std::unique_ptr<CActionUpdater>(new ULambda<CMesh::UndoRedoChanged>([this]()
    {
        m_modelModified = true;
    }));
    m_actionUpdaters.emplace_back(std::move(updater));
}
