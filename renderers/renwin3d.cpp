#include <glm/gtc/matrix_transform.hpp>
#include <QOpenGLWidget>
#include <QEvent>
#include <QMouseEvent>
#include "renwin3d.h"
#include "mesh/mesh.h"

CRenWin3D::CRenWin3D(QWidget *parent) :
    QOpenGLWidget(parent), m_model(nullptr)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setMajorVersion(2);
    format.setMinorVersion(0);
    format.setSamples(2);
    setFormat(format);
    UpdateViewAngles();
}

void CRenWin3D::SetModel(CMesh *mdl)
{
    m_model = mdl;
    ZoomFit();
}

CRenWin3D::~CRenWin3D()
{
    makeCurrent();
    if(m_texture)
        m_texture->destroy();
    doneCurrent();
}

void CRenWin3D::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glShadeModel(GL_FLAT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glm::vec4 diff = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ambi = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, &diff[0]);
    glLightfv(GL_LIGHT1, GL_AMBIENT, &ambi[0]);
}

void CRenWin3D::paintGL()
{
    if(m_model)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMultMatrixf(&m_viewMatrix[0][0]);

        if(m_texture)
            m_texture->bind();
        glBegin(GL_TRIANGLES);

        glLightfv(GL_LIGHT1, GL_POSITION, &m_cameraPosition[0]);

        const std::vector<glm::vec3> &vert = m_model->GetVertices();
        const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();
        const std::vector<glm::vec3> &norms = m_model->GetNormals();

        int i = 0;
        for(const Triangle &t : m_model->GetTriangles())
        {
            //triangle stores info about 3 vertices.
            //vertex info stores 3 indices. 1st - index of position component. 2nd - uv coord index. 3rd - index of normal.
            const glm::uvec3 &vertexInfo1 = t.vertex[0];
            const glm::uvec3 &vertexInfo2 = t.vertex[1];
            const glm::uvec3 &vertexInfo3 = t.vertex[2];

            const glm::vec3 &vertex1 = vert[vertexInfo1[0]-1]; // Indices start from 1!
            const glm::vec3 &vertex2 = vert[vertexInfo2[0]-1];
            const glm::vec3 &vertex3 = vert[vertexInfo3[0]-1];

            const glm::vec2 &uv1 = uvs[vertexInfo1[1]-1];
            const glm::vec2 &uv2 = uvs[vertexInfo2[1]-1];
            const glm::vec2 &uv3 = uvs[vertexInfo3[1]-1];

            const glm::vec3 &faceNormal = norms[i++];

            glNormal3f(faceNormal[0], faceNormal[1], faceNormal[2]);

            glTexCoord2f(uv1[0], uv1[1]);
            glVertex3f(vertex1[0], vertex1[1], vertex1[2]);

            glTexCoord2f(uv2[0], uv2[1]);
            glVertex3f(vertex2[0], vertex2[1], vertex2[2]);

            glTexCoord2f(uv3[0], uv3[1]);
            glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
        }

        glEnd();

        /*
        glEnable(GL_LINE_STIPPLE);
        glLineWidth(5.0f);

        int ti = model->triangles.size() / 2;
        for(int i=0; i<3; ++i)
        {
            if(model->adjacentTriangles[ti].foldType[i] == Mesh::AdjacentInfo::FT_MOUNTAIN)
            {
                glLineStipple(2, 0x11FF);
            } else
            if(model->adjacentTriangles[ti].foldType[i] == Mesh::AdjacentInfo::FT_VALLEY)
            {
                glLineStipple(2, 0x1FFF);
            } else
            {
                glLineStipple(1, 0);
            }
            const glm::vec3 &v1 = model->vertices[model->triangles[ti].vertex[i][0]-1];
            const glm::vec3 &v2 = model->vertices[model->triangles[ti].vertex[(i+1)%3][0]-1];
            glBegin(GL_LINES);
            glVertex3f(v1[0], v1[1], v1[2]);
            glVertex3f(v2[0], v2[1], v2[2]);
            glEnd();
        }
        */

        if(m_texture)
            m_texture->release();
    }
}

void CRenWin3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projMx = glm::perspective(m_fovy, (static_cast<float>(w))/(static_cast<float>(h)), 1.0f, 3000.0f);
    glMultMatrixf(&projMx[0][0]);
}

bool CRenWin3D::event(QEvent *e)
{
    static glm::vec2 oldRot = m_cameraRotation;
    static glm::vec3 oldPos = m_cameraPosition;

    switch(e->type())
    {
        case QEvent::MouseButtonPress :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);

            if(m_cameraMode != CAM_STILL)
            {
                if(m_cameraMode == CAM_ZOOM && me->button() == Qt::LeftButton)
                {
                    m_cameraMode = CAM_TRANSLATE;
                }
                break;
            }

            m_mousePressPoint = me->pos();
            switch(me->button())
            {
                case Qt::MiddleButton :
                {
                    m_cameraMode = CAM_TRANSLATE;
                    oldPos = m_cameraPosition;
                    break;
                }
                case Qt::LeftButton :
                {
                    m_cameraMode = CAM_ROTATE;
                    oldRot = m_cameraRotation;
                    break;
                }
                case Qt::RightButton :
                {
                    m_cameraMode = CAM_ZOOM;
                    oldPos = m_cameraPosition;
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::MouseButtonRelease :
        {
            m_cameraMode = CAM_STILL;
            break;
        }
        case QEvent::MouseMove :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            QPointF newPos = me->pos();
            switch(m_cameraMode)
            {
                case CAM_ZOOM :
                {
                    m_cameraPosition = oldPos + m_front*static_cast<float>(m_mousePressPoint.ry() - newPos.ry())*0.01f;
                    UpdateViewMatrix();
                    break;
                }
                case CAM_ROTATE :
                {
                    m_cameraRotation[0] = oldRot[0] + (newPos.rx() - m_mousePressPoint.rx())*0.001f;
                    m_cameraRotation[1] = oldRot[1] + (newPos.ry() - m_mousePressPoint.ry())*0.001f;
                    UpdateViewAngles();
                    break;
                }
                case CAM_TRANSLATE :
                {
                    m_cameraPosition = oldPos - m_right * static_cast<float>(newPos.rx() - m_mousePressPoint.rx())*0.01f
                                            + m_up    * static_cast<float>(newPos.ry() - m_mousePressPoint.ry())*0.01f;
                    UpdateViewMatrix();
                    break;
                }
                default : break;
            }

            break;
        }
        default : return QWidget::event(e);
    }
    return true;
}

void CRenWin3D::UpdateViewAngles()
{
    static const float _pi = glm::pi<float>();

    if(m_cameraRotation[1] > _pi*0.5f)
        m_cameraRotation[1] = _pi*0.5f;
    else
    if(m_cameraRotation[1] < -_pi*0.5f)
        m_cameraRotation[1] = -_pi*0.5f;

    while(m_cameraRotation[0] > _pi*2.0f)
        m_cameraRotation[0] -= _pi*2.0f;
    while(m_cameraRotation[0] < 0.0f)
        m_cameraRotation[0] += _pi*2.0f;

    m_front = glm::vec3(
        glm::cos(m_cameraRotation[1]) * glm::sin(m_cameraRotation[0]),
        glm::sin(m_cameraRotation[1]),
        glm::cos(m_cameraRotation[1]) * glm::cos(m_cameraRotation[0]));

    m_right = glm::vec3(
        glm::sin(m_cameraRotation[0] - _pi*0.5f),
        0.0f,
        glm::cos(m_cameraRotation[0] - _pi*0.5f));

    m_up = glm::cross(m_right, m_front);
    UpdateViewMatrix();
}

void CRenWin3D::UpdateViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraPosition+m_front, m_up);
    update();
}

void CRenWin3D::LoadTexture(QString path)
{
    makeCurrent();
    m_texture = std::unique_ptr<QOpenGLTexture>(new QOpenGLTexture(QImage(path)));
    m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::Repeat);

    update();
}

void CRenWin3D::ClearTexture()
{
    if(!m_texture)
        return;
    makeCurrent();
    m_texture->destroy();
    m_texture.reset(nullptr);
    update();
}

void CRenWin3D::ZoomFit()
{
    if(!m_model)
        return;
    float lowestX  = 999999999999.0f,
          highestX =-999999999999.0f,
          lowestY  = 999999999999.0f,
          highestY =-999999999999.0f,
          fZ       =-999999999999.0f;

    const glm::vec3* m_aabbox = m_model->GetAABBox();

    glm::mat4 viewMxNOPOS = m_viewMatrix;
    viewMxNOPOS[3] = glm::vec4(0, 0, 0, 1);

    for(int i=0; i<8; ++i)
    {
        glm::vec3 v = glm::vec3(viewMxNOPOS * glm::vec4(m_aabbox[i].x, m_aabbox[i].y, m_aabbox[i].z, 1.0f));
        lowestX = glm::min(lowestX, v.x);
        highestX = glm::max(highestX, v.x);
        lowestY = glm::min(lowestY, v.y);
        highestY = glm::max(highestY, v.y);
        fZ = glm::max(fZ, v.z);
    }
    float oneOverTanHFOVY = 1.0f / glm::tan(glm::radians(m_fovy * 0.5f));
    m_cameraPosition = glm::vec3((lowestX + highestX) * 0.5f, (lowestY + highestY) * 0.5f, 2.0f*(fZ + (highestX - lowestY) * 0.5f * oneOverTanHFOVY));
    m_cameraPosition = glm::vec3(glm::inverse(viewMxNOPOS) * glm::vec4(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f));

    UpdateViewMatrix();
}
