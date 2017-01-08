#include <glm/gtc/matrix_transform.hpp>
#include <QOpenGLWidget>
#include <QEvent>
#include <QMouseEvent>
#include <chrono>
#include <QMessageBox>
#include "settings/settings.h"
#include "renwin3d.h"
#include "mesh/mesh.h"
#include "renderers/renderlegacy3d.h"

CRenWin3D::CRenWin3D(QWidget *parent) :
    IRenWin(parent)
{
    grabKeyboard();
    setMouseTracking(true);

    m_renderer = std::unique_ptr<IRenderer3D>(new CRenderer3DLegacy());
    UpdateViewAngles();
}

CRenWin3D::~CRenWin3D()
{
    makeCurrent(); //set OGL context to properly
    m_renderer.reset(nullptr); //clears textures and other GL resources
    doneCurrent();
}

void CRenWin3D::SetModel(CMesh *mdl)
{
    m_model = mdl;
    m_renderer->SetModel(mdl);
    ZoomFit();
}

void CRenWin3D::LoadTexture(const QImage *img, unsigned index)
{
    makeCurrent();
    m_renderer->LoadTexture(img, index);
    doneCurrent();
    update();
}

void CRenWin3D::ClearTextures()
{
    makeCurrent();
    m_renderer->ClearTextures();
    doneCurrent();
    update();
}

void CRenWin3D::SetEditMode(EditMode mode)
{
    m_editMode = mode;
    m_pickTexValid = false;

    switch(m_editMode)
    {
        case EM_POLYPAINT:
        {
            if(m_model)
                m_model->ClearPickedTriangles();
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
    m_renderer->Init();
}

void CRenWin3D::ToggleLighting(bool checked)
{
    makeCurrent();
    m_renderer->ToggleLighting(checked);
    doneCurrent();
}

void CRenWin3D::ToggleGrid(bool checked)
{
    makeCurrent();
    m_renderer->ToggleGrid(checked);
    doneCurrent();
}

void CRenWin3D::paintGL()
{
    m_renderer->PreDraw();
    m_renderer->DrawScene();
    m_renderer->PostDraw();
}

void CRenWin3D::resizeGL(int w, int h)
{
    m_pickTexValid = false;
    m_width = w;
    m_height = h;
    m_renderer->ResizeView(w, h, m_fovy);
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
                    if(!m_model)
                        break;

                    if(!m_pickTexValid)
                    {
                        try
                        {
                            makeCurrent();
                            m_pickingTexture = m_renderer->GetPickingTexture();
                            doneCurrent();
                        } catch(std::exception& error)
                        {
                            QMessageBox::information(this, "Error", error.what());
                            break;
                        }
                    }

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
                            m_model->SetTriangleAsPicked(index);
                        } else if(mouseKeyFlags & KEY_RMB)
                        {
                            m_model->SetTriangleAsUnpicked(index);
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
    const glm::mat4 viewMatrix = glm::lookAt(m_cameraPosition, m_cameraPosition+m_front, m_up);
    m_renderer->UpdateViewMatrix(viewMatrix);
    update();
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
