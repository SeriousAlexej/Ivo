#include "settings.h"
#include <cassert>

CSettings::CSettings() :
    m_renFlags(R_FLAPS | R_EDGES | R_TEXTR),
    m_papWidth(297u),
    m_papHeight(210u),
    m_resScale(6.0f),
    m_imgFormat(IF_PNG),
    m_imgQuality(100),
    m_lineWidth(1.3f),
    m_stippleLoop(2),
    m_detachAngle(70)
{
}

CSettings& CSettings::GetInstance()
{
    static CSettings s;
    return s;
}

const unsigned char CSettings::GetRenderFlags() const
{
    return m_renFlags;
}

void CSettings::SetRenderFlags(unsigned char aFlags)
{
    m_renFlags = aFlags;
}

const unsigned CSettings::GetPaperWidth() const
{
    return m_papWidth;
}

const unsigned CSettings::GetPaperHeight() const
{
    return m_papHeight;
}

void CSettings::SetPaperWidth(unsigned aWidth)
{
    assert(aWidth > 0);
    m_papWidth = aWidth;
}

void CSettings::SetPaperHeight(unsigned aHeight)
{
    assert(aHeight > 0);
    m_papHeight = aHeight;
}

const float CSettings::GetResolutionScale() const
{
    return m_resScale;
}

void CSettings::SetResolutionScale(float aScale)
{
    assert(aScale >= 1.0f);
    m_resScale = aScale;
}

const CSettings::ImageFormat CSettings::GetImageFormat() const
{
    return m_imgFormat;
}

void CSettings::SetImageFormat(CSettings::ImageFormat aFormat)
{
    m_imgFormat = aFormat;
}

const unsigned char CSettings::GetImageQuality() const
{
    return m_imgQuality;
}

void CSettings::SetImageQuality(unsigned char aQuality)
{
    assert(aQuality <= 100);
    m_imgQuality = aQuality;
}

const float CSettings::GetLineWidth() const
{
    return m_lineWidth;
}

void CSettings::SetLineWidth(float aLineW)
{
    assert(aLineW >= 1.0f);
    m_lineWidth = aLineW;
}

const unsigned CSettings::GetStippleLoop() const
{
    return m_stippleLoop;
}

void CSettings::SetStippleLoop(unsigned aStippleLoop)
{
    assert(aStippleLoop > 0);
    m_stippleLoop = aStippleLoop;
}

const unsigned char CSettings::GetDetachAngle() const
{
    return m_detachAngle;
}

void CSettings::SetDetachAngle(unsigned char aDetachAngle)
{
    assert(aDetachAngle > 0 && aDetachAngle < 360);
    m_detachAngle = aDetachAngle;
}
