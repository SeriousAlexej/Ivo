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
#ifndef SETTINGS_H
#define SETTINGS_H
#include <QObject>
#include <QSettings>
#include <QString>
#include <unordered_map>
#include <string>
#include <vector>
#include "notification/notification.h"

class CSettings : public QObject
{
    Q_OBJECT
public:
    NOTIFICATION(Changed);

    enum ImageFormat
    {
        IF_BMP = 0,
        IF_JPG,
        IF_PNG
    };

    CSettings(const CSettings&) = delete;
    CSettings(CSettings&&) = delete;
    CSettings& operator=(const CSettings&) = delete;
    CSettings& operator=(CSettings&&) = delete;

    ~CSettings();

    static const unsigned char R_FLAPS = (1u << 0);
    static const unsigned char R_EDGES = (1u << 1);
    static const unsigned char R_TEXTR = (1u << 2);
    static const unsigned char R_FOLDS = (1u << 3);
    static const unsigned char R_LIGHT = (1u << 4);
    static const unsigned char R_GRID  = (1u << 5);

    static CSettings&    GetInstance();

    Q_PROPERTY(unsigned char maxFlatAngle READ GetFoldMaxFlatAngle WRITE SetFoldMaxFlatAngle)
    unsigned char        GetFoldMaxFlatAngle() const;
    void                 SetFoldMaxFlatAngle(unsigned char aFoldFlatAngle);

    Q_PROPERTY(unsigned char renderFlags READ GetRenderFlags WRITE SetRenderFlags)
    unsigned char        GetRenderFlags() const;
    void                 SetRenderFlags(unsigned char aFlags);
    void                 SetRenderFlagState(unsigned char aFlag, bool on);

    Q_PROPERTY(unsigned paperWidth READ GetPaperWidth WRITE SetPaperWidth)
    unsigned             GetPaperWidth() const;
    void                 SetPaperWidth(unsigned aWidth);

    Q_PROPERTY(unsigned paperHeight READ GetPaperHeight WRITE SetPaperHeight)
    unsigned             GetPaperHeight() const;
    void                 SetPaperHeight(unsigned aHeight);

    Q_PROPERTY(unsigned horizontalMargins READ GetMarginsHorizontal WRITE SetMarginsHorizontal)
    unsigned             GetMarginsHorizontal() const;
    void                 SetMarginsHorizontal(unsigned aHMargins);

    Q_PROPERTY(unsigned verticalMargins READ GetMarginsVertical WRITE SetMarginsVertical)
    unsigned             GetMarginsVertical() const;
    void                 SetMarginsVertical(unsigned aVMargins);

    Q_PROPERTY(float resolutionScale READ GetResolutionScale WRITE SetResolutionScale)
    float                GetResolutionScale() const;
    void                 SetResolutionScale(float aScale);

    Q_PROPERTY(int imageFormat READ GetImageFormatI WRITE SetImageFormatI)
    ImageFormat          GetImageFormat() const;
    void                 SetImageFormat(ImageFormat aFormat);

    Q_PROPERTY(unsigned char imageQuality READ GetImageQuality WRITE SetImageQuality)
    unsigned char        GetImageQuality() const;
    void                 SetImageQuality(unsigned char aQuality);

    Q_PROPERTY(float lineWidth READ GetLineWidth WRITE SetLineWidth)
    float                GetLineWidth() const;
    void                 SetLineWidth(float aLineW);

    Q_PROPERTY(unsigned stippleLoop READ GetStippleLoop WRITE SetStippleLoop)
    unsigned             GetStippleLoop() const;
    void                 SetStippleLoop(unsigned aStippleLoop);

    Q_PROPERTY(unsigned char detachAngle READ GetDetachAngle WRITE SetDetachAngle)
    unsigned char        GetDetachAngle() const;
    void                 SetDetachAngle(unsigned char aDetachAngle);

    Q_PROPERTY(QString ttStyle     MEMBER ttStyle)
    QString            ttStyle;
    Q_PROPERTY(bool    ttCollapsed MEMBER ttCollapsed)
    bool               ttCollapsed;

private:
    CSettings();

    int GetImageFormatI() const { return (int)GetImageFormat(); }
    void SetImageFormatI(int aFormat) { SetImageFormat((ImageFormat)aFormat); }

    void LoadSettings();
    void SaveSettings();

    QSettings     m_config;

    unsigned char m_renFlags;
    unsigned      m_papWidth;
    unsigned      m_papHeight;
    unsigned      m_marginsHorizontal;
    unsigned      m_marginsVertical;
    float         m_resScale;
    ImageFormat   m_imgFormat;
    unsigned char m_imgQuality;
    float         m_lineWidth;
    unsigned      m_stippleLoop;
    unsigned char m_detachAngle;
    unsigned char m_foldMaxFlatAngle;

    bool          m_loading;
};

#endif // SETTINGS_H

