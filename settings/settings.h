#ifndef SETTINGS_H
#define SETTINGS_H

class CSettings
{
public:
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

    ~CSettings() = default;

    static const unsigned char R_FLAPS = (1u << 0);
    static const unsigned char R_EDGES = (1u << 1);
    static const unsigned char R_TEXTR = (1u << 2);
    static const unsigned char R_FOLDS = (1u << 3);

    static CSettings&   GetInstance();

    unsigned char       GetFoldMaxFlatAngle() const;
    void                SetFoldMaxFlatAngle(unsigned char aFoldFlatAngle);

    unsigned char       GetRenderFlags() const;
    void                SetRenderFlags(unsigned char aFlags);

    unsigned            GetPaperWidth() const;
    void                SetPaperWidth(unsigned aWidth);

    unsigned            GetPaperHeight() const;
    void                SetPaperHeight(unsigned aHeight);

    float               GetResolutionScale() const;
    void                SetResolutionScale(float aScale);

    ImageFormat         GetImageFormat() const;
    void                SetImageFormat(ImageFormat aFormat);

    unsigned char       GetImageQuality() const;
    void                SetImageQuality(unsigned char aQuality);

    float               GetLineWidth() const;
    void                SetLineWidth(float aLineW);

    unsigned            GetStippleLoop() const;
    void                SetStippleLoop(unsigned aStippleLoop);

    unsigned char       GetDetachAngle() const;
    void                SetDetachAngle(unsigned char aDetachAngle);

private:
    CSettings();

    unsigned char m_renFlags;
    unsigned      m_papWidth;
    unsigned      m_papHeight;
    float         m_resScale;
    ImageFormat   m_imgFormat;
    unsigned char m_imgQuality;
    float         m_lineWidth;
    unsigned      m_stippleLoop;
    unsigned char m_detachAngle;
    unsigned char m_foldMaxFlatAngle;
};

#endif // SETTINGS_H

