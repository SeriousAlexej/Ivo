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

    static CSettings& GetInstance();

    const unsigned char GetRenderFlags() const;
    void                SetRenderFlags(unsigned char aFlags);

    const unsigned      GetPaperWidth() const;
    void                SetPaperWidth(unsigned aWidth);

    const unsigned      GetPaperHeight() const;
    void                SetPaperHeight(unsigned aHeight);

    const float         GetResolutionScale() const;
    void                SetResolutionScale(float aScale);

    const ImageFormat   GetImageFormat() const;
    void                SetImageFormat(ImageFormat aFormat);

    const unsigned char GetImageQuality() const;
    void                SetImageQuality(unsigned char aQuality);

    const float         GetLineWidth() const;
    void                SetLineWidth(float aLineW);

    const unsigned      GetStippleLoop() const;
    void                SetStippleLoop(unsigned aStippleLoop);

    const unsigned char GetDetachAngle() const;
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
};

#endif // SETTINGS_H

