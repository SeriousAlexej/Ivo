#include <glm/gtc/matrix_transform.hpp>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <chrono>
#include "settings/settings.h"
#include "renwin3d.h"
#include "mesh/mesh.h"

CRenWin3D::CRenWin3D(QWidget *parent) :
    IRenWin(parent)
{
    grabKeyboard();
    setMouseTracking(true);
    UpdateViewAngles();
}

void CRenWin3D::SetModel(CMesh *mdl)
{
    m_model = mdl;
    ZoomFit();
    m_pickTriIndices.clear();
}

CRenWin3D::~CRenWin3D()
{
}

void CRenWin3D::SetEditMode(EditMode mode)
{
    m_editMode = mode;
    m_pickTexValid = false;

    switch(m_editMode)
    {
        case EM_POLYPAINT:
        {
            m_pickTriIndices.clear();
            break;
        }
        case EM_NONE:
        {
            break;
        }
        default : assert(false);
    }
    update();
}

void CRenWin3D::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glShadeModel(GL_FLAT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glShadeModel(GL_SMOOTH);
    glm::vec4 diff = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ambi = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, &diff[0]);
    glLightfv(GL_LIGHT1, GL_AMBIENT, &ambi[0]);
}

void CRenWin3D::ToggleLighting(bool checked)
{
    m_lighting = checked;
    makeCurrent();
    if(m_lighting)
    {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
    doneCurrent();
}

void CRenWin3D::ToggleGrid(bool checked)
{
    m_grid = checked;
}

void CRenWin3D::DrawAxis()
{
    glDisable(GL_LIGHTING);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-2.0f, 2.0f, -1.5f, 1.5f, 0.1f, 100.0f);

    glm::mat4 rotMx = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), m_front, m_up);

    glm::vec4 vecX(0.1f, 0.0f, 0.0f, 1.0f);
    glm::vec4 vecY(0.0f, 0.1f, 0.0f, 1.0f);
    glm::vec4 vecZ(0.0f, 0.0f, 0.1f, 1.0f);
    vecX = rotMx * vecX;
    vecY = rotMx * vecY;
    vecZ = rotMx * vecZ;

    glTranslatef(-1.9f, -1.4f, -20.0f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecX.x, vecX.y, vecX.z);
    glEnd();
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecY.x, vecY.y, vecY.z);
    glEnd();
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecZ.x, vecZ.y, vecZ.z);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if(m_lighting)
    {
        glEnable(GL_LIGHTING);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
}

void CRenWin3D::DrawGrid()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, 0.0f);
        glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, 0.0f);

        glVertex3f(0.0f, 0.0f, m_cameraPosition.z + 50.0f);
        glVertex3f(0.0f, 0.0f, m_cameraPosition.z - 50.0f);
    glEnd();

    int baseX = (int)m_cameraPosition.x;
    int baseZ = (int)m_cameraPosition.z;

    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(baseX+i, 0.0f, m_cameraPosition.z + 50.0f);
            glVertex3f(baseX+i, 0.0f, m_cameraPosition.z - 50.0f);
        }
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, baseZ+i);
            glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, baseZ+i);
        }
    glEnd();
}

void CRenWin3D::DrawBackground()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if(m_lighting)
        glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glColor3ub(214, 237, 255);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glColor3ub(255, 255, 255);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    if(m_lighting)
        glEnable(GL_LIGHTING);
}

void CRenWin3D::paintGL()
{
    DrawBackground();
    if(m_model)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMultMatrixf(&m_viewMatrix[0][0]);

        const glm::vec4 camPos4v = {m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f};
        glLightfv(GL_LIGHT1, GL_POSITION, &camPos4v[0]);

        const std::vector<glm::vec3> &vert = m_model->GetVertices();
        const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();
        const std::vector<glm::vec3> &norms = m_model->GetNormals();

        glBegin(GL_TRIANGLES);
        int i = 0;
        glColor3ub(255, 255, 255);
        for(const glm::uvec4 &t : m_model->GetTriangles())
        {
            bool faceSelected = m_pickTriIndices.find(i) != m_pickTriIndices.end();

            BindTexture(t[3]);

            const glm::vec3 &vertex1 = vert[t[0]];
            const glm::vec3 &vertex2 = vert[t[1]];
            const glm::vec3 &vertex3 = vert[t[2]];

            const glm::vec2 &uv1 = uvs[t[0]];
            const glm::vec2 &uv2 = uvs[t[1]];
            const glm::vec2 &uv3 = uvs[t[2]];

            const glm::vec3 &faceNormal = norms[i++];

            glNormal3f(faceNormal[0], faceNormal[1], faceNormal[2]);

            if(faceSelected)
                glTexCoord2f(0.0f, 0.0f);
            else
                glTexCoord2f(uv1[0], uv1[1]);
            glVertex3f(vertex1[0], vertex1[1], vertex1[2]);

            if(!faceSelected)
                glTexCoord2f(uv2[0], uv2[1]);
            glVertex3f(vertex2[0], vertex2[1], vertex2[2]);

            if(!faceSelected)
                glTexCoord2f(uv3[0], uv3[1]);
            glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
        }

        glEnd();

        UnbindTexture();
        if(m_grid)
        {
            DrawGrid();
        }
        DrawAxis();
    }
}

void CRenWin3D::resizeGL(int w, int h)
{
    m_pickTexValid = false;
    m_width = w;
    m_height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projMx = glm::perspective(glm::radians(m_fovy), (static_cast<float>(w))/(static_cast<float>(h)), 0.1f, 3000.0f);
    glMultMatrixf(&projMx[0][0]);
}

#define KEY_UP      (1u<<0u)
#define KEY_DOWN    (1u<<1u)
#define KEY_LEFT    (1u<<2u)
#define KEY_RIGHT   (1u<<3u)
#define KEY_FORWARD (1u<<4u)
#define KEY_BACKWRD (1u<<5u)

#define KEY_LMB     (1u<<6u)
#define KEY_RMB     (1u<<7u)

bool CRenWin3D::event(QEvent *e)
{
    static auto lastEventTime = std::chrono::high_resolution_clock::now();
    static unsigned keyFlags = 0u;
    static unsigned mouseKeyFlags = 0u;

    static glm::vec2 oldRot = m_cameraRotation;
    static glm::vec3 oldPos = m_cameraPosition;
    static QPoint oldCursorPos = QCursor::pos();

    if(e->type() == QEvent::User + 1)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastEventTime).count() / 1000.0f;
        lastEventTime = currentTime;

        static const float cameraSpeedCM = 10.0f;//10 cm per second

        if(keyFlags != 0u)
        {
            if(keyFlags & KEY_UP)
            {
                m_cameraPosition += m_up * deltaTime * cameraSpeedCM;
            } else if(keyFlags & KEY_DOWN)
            {
                m_cameraPosition -= m_up * deltaTime * cameraSpeedCM;
            }

            if(keyFlags & KEY_RIGHT)
            {
                m_cameraPosition += m_right * deltaTime * cameraSpeedCM;
            } else if(keyFlags & KEY_LEFT)
            {
                m_cameraPosition -= m_right * deltaTime * cameraSpeedCM;
            }

            if(keyFlags & KEY_FORWARD)
            {
                m_cameraPosition += m_front * deltaTime * cameraSpeedCM;
            } else if(keyFlags & KEY_BACKWRD)
            {
                m_cameraPosition -= m_front * deltaTime * cameraSpeedCM;
            }
            UpdateViewMatrix();
        }

        return true;
    }

    switch(e->type())
    {
        case QEvent::KeyPress :
        {
            if(m_cameraMode != CAM_FLYOVER)
                break;

            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            switch(ke->key())
            {
                case Qt::Key_W :
                {
                    keyFlags |= KEY_FORWARD;
                    break;
                }
                case Qt::Key_S :
                {
                    keyFlags |= KEY_BACKWRD;
                    break;
                }
                case Qt::Key_D :
                {
                    keyFlags |= KEY_RIGHT;
                    break;
                }
                case Qt::Key_A :
                {
                    keyFlags |= KEY_LEFT;
                    break;
                }
                case Qt::Key_Space :
                {
                    keyFlags |= KEY_UP;
                    break;
                }
                case Qt::Key_C :
                {
                    keyFlags |= KEY_DOWN;
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::KeyRelease :
        {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            switch(ke->key())
            {
                case Qt::Key_Escape :
                {
                    keyFlags = 0u;
                    if(m_cameraMode != CAM_FLYOVER && underMouse())
                    {
                        m_cameraMode = CAM_FLYOVER;
                        oldCursorPos = QCursor::pos();
                        grabMouse();
                        setCursor(Qt::BlankCursor);
                    } else {
                        m_cameraMode = CAM_STILL;
                        releaseMouse();
                        setCursor(Qt::ArrowCursor);
                    }
                    break;
                }
                case Qt::Key_W :
                {
                    keyFlags &= ~KEY_FORWARD;
                    break;
                }
                case Qt::Key_S :
                {
                    keyFlags &= ~KEY_BACKWRD;
                    break;
                }
                case Qt::Key_D :
                {
                    keyFlags &= ~KEY_RIGHT;
                    break;
                }
                case Qt::Key_A :
                {
                    keyFlags &= ~KEY_LEFT;
                    break;
                }
                case Qt::Key_Space :
                {
                    keyFlags &= ~KEY_UP;
                    break;
                }
                case Qt::Key_C :
                {
                    keyFlags &= ~KEY_DOWN;
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::Wheel :
        {
            QWheelEvent *we = static_cast<QWheelEvent*>(e);
            m_cameraPosition -= m_front*0.005f*(float)we->delta();
            UpdateViewMatrix();
            break;
        }
        case QEvent::MouseButtonPress :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);

            if(m_cameraMode == CAM_FLYOVER)
                break;

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
                    switch(m_editMode)
                    {
                        case EM_POLYPAINT:
                        {
                            mouseKeyFlags |= KEY_LMB;
                            break;
                        }
                        case EM_NONE:
                        {
                            m_cameraMode = CAM_ROTATE;
                            oldRot = m_cameraRotation;
                            break;
                        }
                        default : assert(false);
                    }
                    break;
                }
                case Qt::RightButton :
                {
                    switch(m_editMode)
                    {
                    case EM_POLYPAINT:
                    {
                        mouseKeyFlags |= KEY_RMB;
                        break;
                    }
                    case EM_NONE:
                    {
                        m_cameraMode = CAM_ZOOM;
                        oldPos = m_cameraPosition;
                        break;
                    }
                    default : assert(false);
                    }
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::MouseButtonRelease :
        {
            if(m_cameraMode == CAM_FLYOVER)
                break;

            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            switch(m_editMode)
            {
                case EM_POLYPAINT:
                {
                    if(me->button() == Qt::LeftButton)
                        mouseKeyFlags &= ~KEY_LMB;
                    else if(me->button() == Qt::RightButton)
                        mouseKeyFlags &= ~KEY_RMB;
                    break;
                }
                case EM_NONE:
                {
                    break;
                }
                default : assert(false);
            }

            m_cameraMode = CAM_STILL;
            break;
        }
        case QEvent::MouseMove :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            QPointF newPos = me->pos();

            switch(m_editMode)
            {
                case EM_POLYPAINT:
                {
                    RefreshPickingTexture();
                    QPoint p = me->pos();

                    if(p.x() < 0 || p.y() < 0 || p.x() >= (int)m_width || p.y() >= (int)m_height)
                        break;

                    QColor col = m_pickingTexture.pixelColor(p.x(), p.y());

                    int index = 0;
                    index |= col.red();
                    index |= col.green() << 8;
                    index |= col.blue() << 16;

                    if(index < 0xFFFFFF)
                    {
                        if(mouseKeyFlags & KEY_LMB)
                        {
                            m_pickTriIndices.insert(index);
                        } else if(mouseKeyFlags & KEY_RMB)
                        {
                            auto foundPos = m_pickTriIndices.find(index);
                            if(foundPos != m_pickTriIndices.end())
                                m_pickTriIndices.erase(foundPos);
                        }
                        update();
                    }
                    break;
                }
                case EM_NONE:
                {
                    break;
                }
                default : assert(false);
            }

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
                case CAM_FLYOVER :
                {
                    QPoint currPos = QCursor::pos();
                    m_cameraRotation[0] -= (currPos.x() - oldCursorPos.x())*0.001f;
                    m_cameraRotation[1] -= (currPos.y() - oldCursorPos.y())*0.001f;
                    UpdateViewAngles();
                    QCursor::setPos(oldCursorPos);
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
    m_pickTexValid = false;
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraPosition+m_front, m_up);
    update();
}

void CRenWin3D::RefreshPickingTexture()
{
    if(m_pickTexValid)
        return;
    m_pickTexValid = true;

    if(!m_model)
        return;

    makeCurrent();

    QOpenGLFramebufferObject fbo(m_width, m_height, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D);
    if(!fbo.isValid())
    {
        return;
    }
    fbo.bind();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&m_viewMatrix[0][0]);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);

    const std::vector<glm::vec3> &vert = m_model->GetVertices();

    int i = 0;
    for(const glm::uvec4 &t : m_model->GetTriangles())
    {
        const glm::vec3 &vertex1 = vert[t[0]];
        const glm::vec3 &vertex2 = vert[t[1]];
        const glm::vec3 &vertex3 = vert[t[2]];

        int r = i & 0x0000FF;
        int g = i & 0x00FF00; g >>= 8;
        int b = i & 0xFF0000; b >>= 16;

        glColor3ub(r, g, b);
        glVertex3f(vertex1[0], vertex1[1], vertex1[2]);
        glVertex3f(vertex2[0], vertex2[1], vertex2[2]);
        glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
        i++;
    }

    glEnd();

    if(m_lighting)
        glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

    m_pickingTexture = fbo.toImage();

    fbo.release();

    doneCurrent();
}

void CRenWin3D::ZoomFit()
{
    if(!m_model)
        return;

    float oneOverTanHFOVY = 1.0f / glm::tan(glm::radians(m_fovy * 0.5f));
    float cameraDistance = m_model->GetBSphereRadius() * oneOverTanHFOVY;
    m_cameraPosition = -cameraDistance * m_front + m_model->GetAABBoxCenter();
    UpdateViewMatrix();
}
