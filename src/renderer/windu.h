#ifndef WINDU_H
#define WINDU_H

#include <mutex>

#include "include_vk.h"
#include <QWindow>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>
#include <QElapsedTimer>
#include <QVector3D>

#include "device.h"
#include "swapchain.h"
#include "renderer.h"
#include "sync.h"
#include "camera.h"
#include "transfer.h"
#include "object_system.h"
#include "resource_manager.h"


class RenderSystem;
class UISystem;

class Windu : public QWindow {
Q_OBJECT
public :
    Windu(QSize resolution);
    ~Windu();

    void render();

    virtual void resizeEvent(QResizeEvent *ev) override;
    virtual void exposeEvent(QExposeEvent *ev) override;
    virtual void keyPressEvent(QKeyEvent *ev) override;
    virtual void keyReleaseEvent(QKeyEvent *ev) override;
    virtual void mouseMoveEvent(QMouseEvent *ev) override;
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    void update();
    virtual bool event(QEvent *e) override;
    void start();
    void reset();
    
    float getTime();

    void prepareGraph();

    QVulkanInstance inst;
    QVulkanFunctions* vki;
    QVulkanDeviceFunctions* vkd;
    vk::DispatchLoaderDynamic* vkip;
    vk::DispatchLoaderDynamic* vkdp;

    Device device;
    Swapchain swap;
    ResourceManager resman;
    Sync sync;
    ObjectSystem objects;
    Transfer transfer;
    Renderer renderer;
    Camera camera;
    RenderSystem* model;
    UISystem* ui;
    
    void setUISystem(UISystem* ui);
    void setRenderSystem(RenderSystem* render);

    QSize size;
    
    bool wasClicked = false;
    bool isClicked = false;
    std::mutex pause;

private :
    bool destroying = false;
    bool loaded = false;
    bool wasResized = true;
    uint32_t i = 0, time = 0;

    QElapsedTimer timer;
    qint64 lastNano = 0, lastHun = 0;
};

#endif
