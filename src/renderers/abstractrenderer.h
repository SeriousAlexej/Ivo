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
#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H
#include <unordered_map>
#include <memory>
#include <QOpenGLTexture>

class QImage;
class CMesh;

class IAbstractRenderer
{
public:
    IAbstractRenderer() = default;
    virtual ~IAbstractRenderer();

    virtual void    SetModel(const CMesh* mdl) = 0;
    virtual void    Init() = 0;

    virtual void    PreDraw() const = 0;
    virtual void    DrawScene() const = 0;
    virtual void    PostDraw() const = 0;

    virtual void    LoadTexture(const QImage* img, unsigned index);
    virtual void    ClearTextures();

protected:
    mutable std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    const CMesh*    m_model = nullptr;
    unsigned        m_width = 800;
    unsigned        m_height = 600;
};


#endif // ABSTRACTRENDERER_H
