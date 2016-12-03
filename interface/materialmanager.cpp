#include <QFileDialog>
#include "materialmanager.h"
#include "ui_materialmanager.h"

CMaterialManager::CMaterialManager(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CMaterialManager)
{
    ui->setupUi(this);
}

CMaterialManager::~CMaterialManager()
{
    delete ui;
}

void CMaterialManager::SetMaterials(const std::unordered_map<unsigned, std::string> &mtls,
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

void CMaterialManager::on_btnClose_clicked()
{
    close();
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
