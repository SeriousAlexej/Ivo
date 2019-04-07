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
#include <QFrame>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonObject>
#include <QFontMetrics>
#include <QShortcut>
#include <TabToolbar/Builder.h>
#include <TabToolbar/TabToolbar.h>
#include <TabToolbar/StyleTools.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings/settings.h"

namespace
{
class LabeledWidgetBase : public QFrame
{
public:
    explicit LabeledWidgetBase(QWidget* parent = nullptr) : QFrame(parent) {}

    virtual QLabel* GetLabel() const = 0;
    virtual QWidget* GetWidget() const = 0;
};

template <class TWidget>
class LabeledWidget : public LabeledWidgetBase
{
public:
    explicit LabeledWidget(const QString& name, QWidget* parent = nullptr) : LabeledWidgetBase(parent)
    {
        setFrameShape(NoFrame);
        setLineWidth(0);
        setContentsMargins(0, 0, 0, 0);

        label = new QLabel(name, this);
        QSpacerItem* spacer = new QSpacerItem(300, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        widget = new TWidget(this);

        QHBoxLayout* innerLayout = new QHBoxLayout(this);
        innerLayout->setMargin(0);
        innerLayout->setContentsMargins(0, 0, 0, 0);
        innerLayout->addWidget(label);
        innerLayout->addItem(spacer);
        innerLayout->addWidget(widget);
        setLayout(innerLayout);
    }
    virtual ~LabeledWidget() = default;

    QWidget* GetWidget() const override
    {
        return widget;
    }

    QLabel* GetLabel() const override
    {
        return label;
    }

private:
    QLabel*  label;
    TWidget* widget;
};
}

void CMainWindow::SetupGUI()
{
    ui->actionExport_Sheets->setShortcut(QKeySequence::Print);
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSave_As->setShortcut(QKeySequence::SaveAs);
    ui->actionAbout->setShortcut(QKeySequence::HelpContents);
    ui->actionLoad_Model->setShortcut(QKeySequence::Open);
    ui->actionOpen_obj->setShortcut(QKeySequence::New);
    ui->actionCloseModel->setShortcut(QKeySequence::Close);
    ui->actionRedo->setShortcut(QKeySequence::Redo);
    ui->actionUndo->setShortcut(QKeySequence::Undo);
    for(QAction* action : findChildren<QAction*>())
        if(!action->shortcut().isEmpty())
        {
            action->setToolTip(action->toolTip() + "    " + QKeySequence(action->shortcut()).toString(QKeySequence::NativeText));
            //make shortcuts's context global
            QShortcut* shortcut = new QShortcut(action->shortcut(), this);
            QObject::connect(shortcut, &QShortcut::activated, this, [action]()
                { if(action->isEnabled()) action->trigger(); });
            action->setShortcut(QKeySequence());
        }

    //force settings to load
    CSettings& sett = CSettings::GetInstance();

    tt::Builder toolbarBuilder(this);
    toolbarBuilder.SetCustomWidgetCreator("spinBox", [this](const QJsonObject& obj)
        { return new LabeledWidget<QSpinBox>(obj["label"].toString(), this); });
    toolbarBuilder.SetCustomWidgetCreator("dspinBox", [this](const QJsonObject& obj)
        { return new LabeledWidget<QDoubleSpinBox>(obj["label"].toString(), this); });

    m_tabToolbar = toolbarBuilder.CreateTabToolbar(":/interface/toolbar.json");
    m_tabToolbar->toggleViewAction()->setText("Tabbed Toolbar");
    addToolBar(Qt::TopToolBarArea, m_tabToolbar);
    if(!sett.ttStyle.isEmpty())
        QTimer::singleShot(0, this, [this]() { m_tabToolbar->SetStyle(CSettings::GetInstance().ttStyle); });
    if(sett.ttCollapsed)
        QTimer::singleShot(0, this, [this]() { m_tabToolbar->HideAction()->trigger(); });
    UpdateStyle();

    m_rw2->SetContextMenu((QMenu*)(toolbarBuilder["2DMenu"]));

    //create themes menu
    QMenu* themesMenu = (QMenu*)(toolbarBuilder["themesMenu"]);
    QList<QAction*> styleActions;
    const QString currStyle = m_tabToolbar->GetStyle();
    QActionGroup* styleGroup = new QActionGroup(this);
    for(const QString& style : tt::GetRegisteredStyles())
    {
        QAction* styleAction = new QAction(style, this);
        styleAction->setCheckable(true);
        styleAction->setChecked(style == currStyle);
        QObject::connect(styleAction, &QAction::triggered, [this, style]
            { m_tabToolbar->SetStyle(style); UpdateStyle(); });
        QObject::connect(m_tabToolbar, &tt::TabToolbar::StyleChanged, styleAction, [this, styleAction]
            { styleAction->setChecked(styleAction->text() == m_tabToolbar->GetStyle()); });
        styleActions.append(styleAction);
        styleGroup->addAction(styleAction);
    }
    themesMenu->addActions(styleActions);

    //adjust horizontal size
    auto horItems = {"maxFlatAngle", "lineWidth", "stippleFactor"};
    int maxWidth = 0;
    const int spinboxWidth = 50;
    for(const char* item : horItems)
    {
        auto* widget = static_cast<LabeledWidgetBase*>(toolbarBuilder[item]);
        QFontMetrics metrics(widget->GetLabel()->font());
        maxWidth = std::max(maxWidth, metrics.width(widget->GetLabel()->text()));
    }
    for(const char* item : horItems)
    {
        auto* widget = static_cast<LabeledWidgetBase*>(toolbarBuilder[item]);
        widget->GetWidget()->setFixedWidth(spinboxWidth);
        widget->setMaximumWidth(maxWidth + spinboxWidth + 5);
    }

    //setup custom widgets
    auto* widgetBase = static_cast<LabeledWidgetBase*>(toolbarBuilder["lineWidth"]);
    QDoubleSpinBox* lineWidth = static_cast<QDoubleSpinBox*>(widgetBase->GetWidget());
    lineWidth->setSingleStep(0.1);
    lineWidth->setMinimum(0.1);
    lineWidth->setMaximum(50.0);
    lineWidth->setDecimals(1);
    lineWidth->setValue(sett.GetLineWidth());
    QObject::connect(lineWidth, (void(QDoubleSpinBox::*)(double))&QDoubleSpinBox::valueChanged, [&sett](double d)
        { sett.SetLineWidth(d); });

    widgetBase = static_cast<LabeledWidgetBase*>(toolbarBuilder["maxFlatAngle"]);
    QSpinBox* maxFlatAngle = static_cast<QSpinBox*>(widgetBase->GetWidget());
    maxFlatAngle->setSingleStep(1);
    maxFlatAngle->setMinimum(0);
    maxFlatAngle->setMaximum(179);
    maxFlatAngle->setValue(sett.GetFoldMaxFlatAngle());
    QObject::connect(maxFlatAngle, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, [&sett](int i)
        { sett.SetFoldMaxFlatAngle(i); });

    widgetBase = static_cast<LabeledWidgetBase*>(toolbarBuilder["stippleFactor"]);
    QSpinBox* stippleFactor = static_cast<QSpinBox*>(widgetBase->GetWidget());
    stippleFactor->setSingleStep(1);
    stippleFactor->setMinimum(1);
    stippleFactor->setMaximum(10);
    stippleFactor->setValue(sett.GetStippleLoop());
    QObject::connect(stippleFactor, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, [&sett](int i)
        { sett.SetStippleLoop(i); });
}
