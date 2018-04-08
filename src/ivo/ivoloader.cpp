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
#include <QString>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "interface/mainwindow.h"
#include "mesh/mesh.h"
#include "settings/settings.h"

void CMainWindow::SaveToIVO(const QString& filename)
{
    const CSettings& sett = CSettings::GetInstance();

    QJsonObject root;
    root.insert("version", IVO_VERSION);
    root.insert("renderFlags", sett.GetRenderFlags());
    root.insert("paperWidth", static_cast<int>(sett.GetPaperWidth()));
    root.insert("paperHeight", static_cast<int>(sett.GetPaperHeight()));
    root.insert("marginsH", static_cast<int>(sett.GetMarginsHorizontal()));
    root.insert("marginsV", static_cast<int>(sett.GetMarginsVertical()));
    root.insert("resolutionScale", sett.GetResolutionScale());
    root.insert("imageFormat", sett.GetImageFormat());
    root.insert("imageQuality", sett.GetImageQuality());
    root.insert("lineWidth", sett.GetLineWidth());
    root.insert("stippleLoop", static_cast<int>(sett.GetStippleLoop()));
    root.insert("maxFlatAngle", sett.GetFoldMaxFlatAngle());
    root.insert("mesh", m_model->Serialize());

    QJsonArray matArray;
    auto materials = m_model->GetMaterials();
    for(auto it=materials.begin(); it!=materials.end(); it++)
    {
        auto index = static_cast<int>(it->first);
        std::string matName = it->second;
        std::string texPath = m_textures[index];
        const QImage* image = m_textureImages[index].get();

        QJsonObject matEntry;
        matEntry.insert("index", index);
        matEntry.insert("name", matName.c_str());
        matEntry.insert("path", texPath.c_str());

        if(image)
        {
            QJsonObject imageObject;
            imageObject.insert("width", image->width());
            imageObject.insert("height", image->height());
            imageObject.insert("format", static_cast<int>(image->format()));

            const auto imageData = qCompress(image->constBits(), image->byteCount(), 9);
            imageObject.insert("base64CompressedPixelData", QString(imageData.toBase64()));

            matEntry.insert("image", imageObject);
        }
        else
        {
            matEntry.insert("image", QJsonValue());
        }
        matArray.append(matEntry);
    }
    root.insert("materials", matArray);

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Error", QString("Failed to open '") + filename + "' for writing.");
        return;
    }
    QJsonDocument doc;
    doc.setObject(root);
    const QByteArray docData = doc.toJson(QJsonDocument::Indented);
    auto writtenSize = file.write(docData);
    if(writtenSize != docData.size())
    {
        QMessageBox::warning(this, "Error", QString("Failed to write data to file! Contents might be corrupted."));
    }
    else
    {
        m_openedModel = filename;
        m_modelModified = false;
    }
}

void CMainWindow::LoadFromIVO(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Error", QString("Failed to open '") + filename + "' for reading.");
        return;
    }
    QJsonParseError jsonError;
    const QJsonDocument doc(QJsonDocument::fromJson(file.readAll(), &jsonError));
    if(jsonError.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(this, "Error", QString("Parse error: ") + jsonError.errorString());
        return;
    }
    file.close();

    const QJsonObject root = doc.object();
    const int version = root["version"].toInt();
    switch(version)
    {
        case 1:
        {
            std::unique_ptr<CMesh> newModel(new CMesh());
            newModel->Deserialize(root["mesh"].toObject());

            m_openedModel = filename;

            m_model = std::move(newModel);
            SetModelToWindows();
            ClearTextures();

            std::unordered_map<unsigned, std::string> materials;
            const QJsonArray materialsArray = root["materials"].toArray();
            for(int i=0; i<materialsArray.size(); ++i)
            {
                const QJsonObject material = materialsArray.at(i).toObject();
                const auto index = static_cast<unsigned>(material["index"].toInt());

                materials[index] = material["name"].toString().toStdString();
                m_textures[index] = material["path"].toString().toStdString();

                if(!material["image"].isNull())
                {
                    const QJsonObject image = material["image"].toObject();
                    const int texWidth = image["width"].toInt();
                    const int texHeight = image["height"].toInt();
                    const int texFormat = image["format"].toInt();
                    QByteArray imageData;
                    imageData.append(image["base64CompressedPixelData"].toString());
                    imageData = qUncompress(QByteArray::fromBase64(imageData));

                    m_textureImages[index].reset(new QImage(
                                                     reinterpret_cast<const uchar*>(imageData.constData()),
                                                     texWidth,
                                                     texHeight,
                                                     static_cast<QImage::Format>(texFormat)));
                    (*m_textureImages[index]) = m_textureImages[index]->copy();

                    emit UpdateTexture(m_textureImages[index].get(), index);
                }

            }
            m_model->SetMaterials(materials);

            CSettings& sett = CSettings::GetInstance();
            sett.SetRenderFlags(root["renderFlags"].toInt());
            sett.SetPaperWidth(root["paperWidth"].toInt());
            sett.SetPaperHeight(root["paperHeight"].toInt());
            sett.SetMarginsHorizontal(root["marginsH"].toInt());
            sett.SetMarginsVertical(root["marginsV"].toInt());
            sett.SetResolutionScale(root["resolutionScale"].toDouble());
            sett.SetImageFormat(static_cast<CSettings::ImageFormat>(root["imageFormat"].toInt()));
            sett.SetImageQuality(root["imageQuality"].toInt());
            sett.SetLineWidth(root["lineWidth"].toDouble());
            sett.SetStippleLoop(root["stippleLoop"].toInt());
            sett.SetFoldMaxFlatAngle(root["maxFlatAngle"].toInt());
            break;
        }
        default :
            QMessageBox::information(this, "Error", "Ivo format version " + QString::number(version) + " is not supported by this version of program!");
    }
}
