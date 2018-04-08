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
#include <cassert>
#include <QTimer>
#include <QMetaProperty>
#include "settings.h"
#include "notification/hub.h"

CSettings::CSettings() :
    QObject(nullptr),
    ttStyle(""),
    ttCollapsed(false),
    m_config("config.ini", QSettings::IniFormat),
    m_renFlags(R_FLAPS | R_EDGES | R_TEXTR | R_FOLDS | R_LIGHT | R_GRID),
    m_papWidth(297u),
    m_papHeight(210u),
    m_marginsHorizontal(10u),
    m_marginsVertical(10u),
    m_resScale(6.0f),
    m_imgFormat(IF_PNG),
    m_imgQuality(100),
    m_lineWidth(1.0f),
    m_stippleLoop(2),
    m_detachAngle(70),
    m_foldMaxFlatAngle(1),
    m_loading(false)
{
    LoadSettings();
}

CSettings::~CSettings()
{
    SaveSettings();
}

void CSettings::LoadSettings()
{
    m_loading = true;

    const int numProps = metaObject()->propertyCount();
    for(int i=0; i<numProps; i++)
    {
        const QMetaProperty prop = metaObject()->property(i);
        const QString propName = prop.name();
        if(propName == "objectName")
            continue;
        const QVariant val = m_config.value(propName);
        if(val.isValid())
            setProperty(prop.name(), val);
    }
    m_config.sync();

    m_loading = false;
}

void CSettings::SaveSettings()
{
    const int numProps = metaObject()->propertyCount();
    for(int i=0; i<numProps; i++)
    {
        const QMetaProperty prop = metaObject()->property(i);
        const QString propName = prop.name();
        if(propName == "objectName")
            continue;
        m_config.setValue(propName, property(prop.name()));
    }
    m_config.sync();
}

CSettings& CSettings::GetInstance()
{
    static CSettings s;
    return s;
}

unsigned char CSettings::GetRenderFlags() const
{
    return m_renFlags;
}

void CSettings::SetRenderFlags(unsigned char aFlags)
{
    m_renFlags = aFlags;
    if(!m_loading)
        NOTIFY(Changed);
}

void CSettings::SetRenderFlagState(unsigned char aFlag, bool on)
{
    if(on)
        SetRenderFlags(GetRenderFlags() |  aFlag);
    else
        SetRenderFlags(GetRenderFlags() & ~aFlag);
}

unsigned CSettings::GetPaperWidth() const
{
    return m_papWidth;
}

unsigned CSettings::GetPaperHeight() const
{
    return m_papHeight;
}

void CSettings::SetPaperWidth(unsigned aWidth)
{
    assert(aWidth > 0);
    m_papWidth = aWidth;
    if(!m_loading)
        NOTIFY(Changed);
}

void CSettings::SetPaperHeight(unsigned aHeight)
{
    assert(aHeight > 0);
    m_papHeight = aHeight;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned CSettings::GetMarginsHorizontal() const
{
    return m_marginsHorizontal;
}

void CSettings::SetMarginsHorizontal(unsigned aHMargins)
{
    assert(aHMargins > 0);
    m_marginsHorizontal = aHMargins;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned CSettings::GetMarginsVertical() const
{
    return m_marginsVertical;
}

void CSettings::SetMarginsVertical(unsigned aVMargins)
{
    assert(aVMargins > 0);
    m_marginsVertical = aVMargins;
    if(!m_loading)
        NOTIFY(Changed);
}

float CSettings::GetResolutionScale() const
{
    return m_resScale;
}

void CSettings::SetResolutionScale(float aScale)
{
    assert(aScale >= 1.0f);
    m_resScale = aScale;
    if(!m_loading)
        NOTIFY(Changed);
}

CSettings::ImageFormat CSettings::GetImageFormat() const
{
    return m_imgFormat;
}

void CSettings::SetImageFormat(CSettings::ImageFormat aFormat)
{
    m_imgFormat = aFormat;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned char CSettings::GetImageQuality() const
{
    return m_imgQuality;
}

void CSettings::SetImageQuality(unsigned char aQuality)
{
    assert(aQuality <= 100);
    m_imgQuality = aQuality;
    if(!m_loading)
        NOTIFY(Changed);
}

float CSettings::GetLineWidth() const
{
    return m_lineWidth;
}

void CSettings::SetLineWidth(float aLineW)
{
    assert(aLineW >= 0.1f);
    m_lineWidth = aLineW;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned CSettings::GetStippleLoop() const
{
    return m_stippleLoop;
}

void CSettings::SetStippleLoop(unsigned aStippleLoop)
{
    assert(aStippleLoop > 0);
    m_stippleLoop = aStippleLoop;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned char CSettings::GetDetachAngle() const
{
    return m_detachAngle;
}

void CSettings::SetDetachAngle(unsigned char aDetachAngle)
{
    m_detachAngle = aDetachAngle;
    if(!m_loading)
        NOTIFY(Changed);
}

unsigned char CSettings::GetFoldMaxFlatAngle() const
{
    return m_foldMaxFlatAngle;
}

void CSettings::SetFoldMaxFlatAngle(unsigned char aFoldFlatAngle)
{
    assert(aFoldFlatAngle < 180);
    m_foldMaxFlatAngle = aFoldFlatAngle;
    if(!m_loading)
        NOTIFY(Changed);
}
