#include <QMessageBox>
#include "interface/mainwindow.h"
#include "io/saferead.h"
#include "mesh/mesh.h"
#include "settings/settings.h"
#include "interface/renwin.h"
#include "interface/renwin2d.h"

void CMainWindow::SaveToIVO(const char* filename)
{
    FILE* f = std::fopen(filename, "wb");
    if(!f)
        return;

    char ivo[4] = "IVO";
    int version = IVO_VERSION;

    unsigned char renFlags = CSettings::GetInstance().GetRenderFlags();
    unsigned      paperWid = CSettings::GetInstance().GetPaperWidth();
    unsigned      paperHei = CSettings::GetInstance().GetPaperHeight();
    float         resScale = CSettings::GetInstance().GetResolutionScale();
    int           imageFmt = CSettings::GetInstance().GetImageFormat();
    unsigned char imageQlt = CSettings::GetInstance().GetImageQuality();
    float         lineWidt = CSettings::GetInstance().GetLineWidth();
    unsigned      stipplLp = CSettings::GetInstance().GetStippleLoop();
    unsigned char maxFlAng = CSettings::GetInstance().GetFoldMaxFlatAngle();


    //header + version
    std::fwrite(ivo, sizeof(char), 3, f);
    std::fwrite(&version, sizeof(int), 1, f);
    //settings
    std::fwrite(&renFlags, sizeof(renFlags), 1, f);
    std::fwrite(&paperWid, sizeof(paperWid), 1, f);
    std::fwrite(&paperHei, sizeof(paperHei), 1, f);
    std::fwrite(&resScale, sizeof(resScale), 1, f);
    std::fwrite(&imageFmt, sizeof(imageFmt), 1, f);
    std::fwrite(&imageQlt, sizeof(imageQlt), 1, f);
    std::fwrite(&lineWidt, sizeof(lineWidt), 1, f);
    std::fwrite(&stipplLp, sizeof(stipplLp), 1, f);
    std::fwrite(&maxFlAng, sizeof(maxFlAng), 1, f);
    //mesh data
    m_model->Serialize(f);

    m_rw2->SerializeSheets(f);

    auto materials = m_model->GetMaterials();
    unsigned char numTextures = (unsigned char)materials.size();
    std::fwrite(&numTextures, sizeof(numTextures), 1, f);
    for(auto it=materials.begin(); it!=materials.end(); it++)
    {
        unsigned key = it->first;
        std::string matName = it->second;
        std::string texPath = m_textures[key];
        const QImage* image = m_textureImages[key].get();

        int nameLen = matName.length();
        int pathLen = texPath.length();

        std::fwrite(&key, sizeof(key), 1, f);
        std::fwrite(&nameLen, sizeof(nameLen), 1, f);
        std::fwrite(matName.data(), sizeof(char), nameLen, f);
        std::fwrite(&pathLen, sizeof(pathLen), 1, f);
        std::fwrite(texPath.data(), sizeof(char), pathLen, f);

        int hasTexture = (image != nullptr ? 1 : 0);
        std::fwrite(&hasTexture, sizeof(hasTexture), 1, f);

        if(image)
        {
            int texWidth = image->width();
            int texHeight = image->height();
            int texByteCount = image->byteCount();
            int texFormat = (int)image->format();

            std::fwrite(&texWidth, sizeof(texWidth), 1, f);
            std::fwrite(&texHeight, sizeof(texHeight), 1, f);
            std::fwrite(&texByteCount, sizeof(texByteCount), 1, f);
            std::fwrite(&texFormat, sizeof(texFormat), 1, f);
            std::fwrite(image->constBits(), sizeof(unsigned char), texByteCount, f);
        }
    }

    m_openedModel = filename;

    std::fclose(f);
}

void CMainWindow::LoadFromIVO(const char* filename)
{
    FILE* f = std::fopen(filename, "rb");
    if(!f)
        SafeRead::BadFile(f);

    char ivo[4];
    int version = -1;

    SAFE_FREAD(ivo, sizeof(char), 3, f);
    ivo[3] = '\0';
    if(strcmp(ivo, "IVO") != 0)
    {
        QMessageBox::information(this, "Error", "Selected file is not a valid IVO model!");
        std::fclose(f);
        return;
    }
    SAFE_FREAD(&version, sizeof(version), 1, f);

    switch(version)
    {
        case 1 :
        {
            unsigned char renFlags;
            unsigned      paperWid;
            unsigned      paperHei;
            float         resScale;
            int           imageFmt;
            unsigned char imageQlt;
            float         lineWidt;
            unsigned      stipplLp;
            unsigned char maxFlAng;

            SAFE_FREAD(&renFlags, sizeof(renFlags), 1, f);
            SAFE_FREAD(&paperWid, sizeof(paperWid), 1, f);
            SAFE_FREAD(&paperHei, sizeof(paperHei), 1, f);
            SAFE_FREAD(&resScale, sizeof(resScale), 1, f);
            SAFE_FREAD(&imageFmt, sizeof(imageFmt), 1, f);
            SAFE_FREAD(&imageQlt, sizeof(imageQlt), 1, f);
            SAFE_FREAD(&lineWidt, sizeof(lineWidt), 1, f);
            SAFE_FREAD(&stipplLp, sizeof(stipplLp), 1, f);
            SAFE_FREAD(&maxFlAng, sizeof(maxFlAng), 1, f);

            std::unique_ptr<CMesh> newModelUP(new CMesh());
            newModelUP->Deserialize(f);
            CMesh* newModel = newModelUP.release();

            m_openedModel = filename;

            m_rw2->SetModel(newModel);
            ((IRenWin*)m_rw3)->SetModel(newModel);
            if(m_model)
                delete m_model;
            m_model = newModel;
            ClearTextures();

            m_rw2->DeserializeSheets(f);
            m_rw2->UpdateSheetsSize();

            std::unordered_map<unsigned, std::string> materials;

            unsigned char numTextures = 0;
            SAFE_FREAD(&numTextures, sizeof(numTextures), 1, f);
            for(int i=numTextures-1; i>=0; i--)
            {
                unsigned key;
                int nameLen;
                int pathLen;
                SAFE_FREAD(&key, sizeof(key), 1, f);

                SAFE_FREAD(&nameLen, sizeof(nameLen), 1, f);
                std::unique_ptr<char[]> name(new char[nameLen+1]);
                name[nameLen] = '\0';
                SAFE_FREAD(name.get(), sizeof(char), nameLen, f);

                SAFE_FREAD(&pathLen, sizeof(pathLen), 1, f);
                std::unique_ptr<char[]> path(new char[pathLen+1]);
                path[pathLen] = '\0';
                SAFE_FREAD(path.get(), sizeof(char), pathLen, f);

                int hasTexture = 0;
                SAFE_FREAD(&hasTexture, sizeof(hasTexture), 1, f);

                if(hasTexture == 1)
                {
                    int texWidth = 0;
                    int texHeight = 0;
                    int texByteCount = 0;
                    int texFormat = 0;

                    SAFE_FREAD(&texWidth, sizeof(texWidth), 1, f);
                    SAFE_FREAD(&texHeight, sizeof(texHeight), 1, f);
                    SAFE_FREAD(&texByteCount, sizeof(texByteCount), 1, f);
                    SAFE_FREAD(&texFormat, sizeof(texFormat), 1, f);

                    std::unique_ptr<unsigned char[]> imgBits(new unsigned char[texByteCount]);
                    SAFE_FREAD(imgBits.get(), sizeof(unsigned char), texByteCount, f);

                    m_textureImages[key].reset(new QImage(imgBits.get(), texWidth, texHeight, (QImage::Format)texFormat));
                    (*m_textureImages[key]) = m_textureImages[key]->copy();
                }

                materials[key] = name.get();
                m_textures[key] = path.get();

                if(hasTexture == 1)
                    emit UpdateTexture(m_textureImages[key].get(), key);
            }

            m_model->SetMaterials(materials);

            CSettings& sett = CSettings::GetInstance();

            sett.SetRenderFlags( renFlags );
            sett.SetPaperWidth( paperWid );
            sett.SetPaperHeight( paperHei );
            sett.SetResolutionScale( resScale );
            sett.SetImageFormat( (CSettings::ImageFormat)imageFmt );
            sett.SetImageQuality( imageQlt );
            sett.SetLineWidth( lineWidt );
            sett.SetStippleLoop( stipplLp );
            sett.SetFoldMaxFlatAngle( maxFlAng );
            m_rw2->UpdateSheetsSize();

            break;
        }
        default :
        {
            QMessageBox::information(this, "Error", "Ivo format version " + QString::number(version) + " is not supported by this version of program!");
        }
    }

    std::fclose(f);
}
