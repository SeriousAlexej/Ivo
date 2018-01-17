#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H

#include <QDialog>
#include <unordered_map>
#include <string>

namespace Ui {
class CMaterialManager;
}

class CMaterialManager : public QDialog
{
    Q_OBJECT

public:
    explicit CMaterialManager(QWidget* parent = nullptr);
    ~CMaterialManager();

    void SetMaterials(const std::unordered_map<unsigned, std::string>& mtls,
                      const std::unordered_map<unsigned, std::string>& txtrs);

    inline std::unordered_map<unsigned, std::string>& GetTextures() { return m_textures; }

private slots:
    void on_btnAddTex_clicked();
    void on_btnRemTex_clicked();
    void on_listMaterials_itemSelectionChanged();

private:
    Ui::CMaterialManager *ui;

    std::unordered_map<unsigned, std::string> m_materials;
    std::unordered_map<unsigned, std::string> m_textures;
};

#endif // MATERIALMANAGER_H
