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
#include <QFileDialog>
#include <QDialogButtonBox>
#include "materialmanager.h"
#include "ui_materialmanager.h"

CMaterialManager::CMaterialManager(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CMaterialManager)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QObject::connect(ui->btnClose, &QDialogButtonBox::clicked, this, &QDialog::close);
}

CMaterialManager::~CMaterialManager()
{
    delete ui;
}

void CMaterialManager::SetMaterials(const std::unordered_map<unsigned, std::string>& mtls,
                                    const std::unordered_map<unsigned, std::string>& txtrs)
{
    if(&m_materials != &mtls)
        m_materials = mtls;
    if(&m_textures != &txtrs)
        m_textures = txtrs;

    ui->listMaterials->clear();

    for(auto it=m_materials.begin(); it!=m_materials.end(); it++)
    {
        std::string texPath = m_textures[it->first];
        if(texPath.empty())
            texPath = "<no texture>";
        auto slashPos = texPath.find_last_of('/');
        if(slashPos != std::string::npos)
            texPath = texPath.substr(slashPos+1);
        slashPos = texPath.find_last_of('\\');
        if(slashPos != std::string::npos)
            texPath = texPath.substr(slashPos+1);

        ui->listMaterials->addItem(QString((it->second + " -- " + texPath).c_str()));
    }
}

void CMaterialManager::on_btnAddTex_clicked()
{
    QListWidgetItem* selectedMaterial = ui->listMaterials->currentItem();
    if(!selectedMaterial)
        return;

    QString texturePath = QFileDialog::getOpenFileName(this,
                                                       "Load Texture",
                                                       "",
                                                       "Image file (*.png *.bmp *.jpg)");
    if(!texturePath.isEmpty())
    {
        std::string selectedMatName = selectedMaterial->text().toStdString();
        selectedMatName = selectedMatName.substr(0, selectedMatName.find(" -- "));
        for(auto it=m_materials.begin(); it!=m_materials.end(); it++)
        {
            if(it->second == selectedMatName)
            {
                m_textures[it->first] = texturePath.toStdString();
                break;
            }
        }
    }
    SetMaterials(m_materials, m_textures);
}

void CMaterialManager::on_btnRemTex_clicked()
{
    QListWidgetItem* selectedMaterial = ui->listMaterials->currentItem();
    if(!selectedMaterial)
        return;

    std::string selectedMatName = selectedMaterial->text().toStdString();
    selectedMatName = selectedMatName.substr(0, selectedMatName.find(" -- "));
    for(auto it=m_materials.begin(); it!=m_materials.end(); it++)
    {
        if(it->second == selectedMatName)
        {
            m_textures[it->first] = "";
            break;
        }
    }
    SetMaterials(m_materials, m_textures);
}

void CMaterialManager::on_listMaterials_itemSelectionChanged()
{
    bool enable = ui->listMaterials->currentItem() != nullptr;
    ui->btnAddTex->setEnabled(enable);
    ui->btnRemTex->setEnabled(enable);
}
