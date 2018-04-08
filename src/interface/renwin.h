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
#ifndef IRENWIN_H
#define IRENWIN_H
#include <QOpenGLWidget>

class CMesh;

class IRenWin : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit IRenWin(QWidget *parent = nullptr);
    virtual ~IRenWin() = default;

    virtual void SetModel(CMesh *mdl) = 0;

    virtual void ZoomFit() = 0;

public slots:
    virtual void LoadTexture(const QImage *img, unsigned index) = 0;
    virtual void ClearTextures() = 0;

signals:
    void         RequestFullRedraw();

protected:
    CMesh*       m_model;
};

#endif // IRENWIN_H
