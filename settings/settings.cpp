#include "settings.h"
#include <cassert>
#include <QTextStream>
#include <QFile>
#include <QTimer>
#include <QFileInfo>
#include <QDirIterator>

CSettings::CSettings() :
    QObject(nullptr),
    m_config("config.ini", QSettings::IniFormat),
    m_renFlags(R_FLAPS | R_EDGES | R_TEXTR | R_FOLDS),
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
    m_foldMaxFlatAngle(1)
{
    LoadStyles();
    QTimer::singleShot(0, [this]() { LoadSettings(); });
}

void CSettings::LoadSettings()
{
    SetActiveStyle(GetActiveStyle());
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
}

void CSettings::SetPaperHeight(unsigned aHeight)
{
    assert(aHeight > 0);
    m_papHeight = aHeight;
}

unsigned CSettings::GetMarginsHorizontal() const
{
    return m_marginsHorizontal;
}

void CSettings::SetMarginsHorizontal(unsigned aHMargins)
{
    assert(aHMargins > 0);
    m_marginsHorizontal = aHMargins;
}

unsigned CSettings::GetMarginsVertical() const
{
    return m_marginsVertical;
}

void CSettings::SetMarginsVertical(unsigned aVMargins)
{
    assert(aVMargins > 0);
    m_marginsVertical = aVMargins;
}

float CSettings::GetResolutionScale() const
{
    return m_resScale;
}

void CSettings::SetResolutionScale(float aScale)
{
    assert(aScale >= 1.0f);
    m_resScale = aScale;
}

CSettings::ImageFormat CSettings::GetImageFormat() const
{
    return m_imgFormat;
}

void CSettings::SetImageFormat(CSettings::ImageFormat aFormat)
{
    m_imgFormat = aFormat;
}

unsigned char CSettings::GetImageQuality() const
{
    return m_imgQuality;
}

void CSettings::SetImageQuality(unsigned char aQuality)
{
    assert(aQuality <= 100);
    m_imgQuality = aQuality;
}

float CSettings::GetLineWidth() const
{
    return m_lineWidth;
}

void CSettings::SetLineWidth(float aLineW)
{
    assert(aLineW >= 0.1f);
    m_lineWidth = aLineW;
}

unsigned CSettings::GetStippleLoop() const
{
    return m_stippleLoop;
}

void CSettings::SetStippleLoop(unsigned aStippleLoop)
{
    assert(aStippleLoop > 0);
    m_stippleLoop = aStippleLoop;
}

unsigned char CSettings::GetDetachAngle() const
{
    return m_detachAngle;
}

void CSettings::SetDetachAngle(unsigned char aDetachAngle)
{
    m_detachAngle = aDetachAngle;
}

unsigned char CSettings::GetFoldMaxFlatAngle() const
{
    return m_foldMaxFlatAngle;
}

void CSettings::SetFoldMaxFlatAngle(unsigned char aFoldFlatAngle)
{
    assert(aFoldFlatAngle < 180);
    m_foldMaxFlatAngle = aFoldFlatAngle;
}

std::vector<QString> CSettings::GetStyleNames() const
{
    std::vector<QString> styleNames;
    styleNames.reserve(m_styles.size());
    for(auto style = m_styles.cbegin(); style != m_styles.cend(); style++)
        styleNames.push_back(style->first.c_str());
    return styleNames;
}

QString CSettings::GetActiveStyle() const
{
    return m_config.value("Style", QString("Native")).toString();
}

void CSettings::SetActiveStyle(const QString& style)
{
    auto found = m_styles.find(style.toStdString());
    assert(found != m_styles.end());
    m_config.setValue("Style", style);
    emit SetAppStyle(found->second);
}

void CSettings::LoadStyles()
{
    m_styles.clear();
    m_styles["Native"] = "";

    QDirIterator it(":/themes/styles", QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        const QFileInfo styleFile = it.next();
        QFile file(styleFile.absoluteFilePath());
        if (!file.open(QFile::ReadOnly | QFile::Text))
            continue;
        QTextStream stream(&file);
        m_styles[styleFile.baseName().toStdString()] = stream.readAll();
    }
}
