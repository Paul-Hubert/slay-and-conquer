#include "windu.h"

#include "logic/systems/render_system.h"
#include "logic/systems/ui_system.h"

#include <stdio.h>
#include <QResizeEvent>
#include <iostream>
#include <QSurface>
#include <QApplication>
#include <QSize>

#include "helper.h"
#include "util/settings.h"

Windu::Windu(QSize resolution) : device(this), swap(this), resman(this), sync(this), objects(this), transfer(this), renderer(this), size(resolution) {

    VkResult result = volkInitialize();

    if(result != VK_SUCCESS) {
         qFatal("Failed to load Vulkan Loader");
    }

    setSurfaceType(SurfaceType::VulkanSurface);

    inst.setApiVersion(QVersionNumber(1, 0, 0));

    inst.setLayers(QByteArrayList()
                   << "VK_LAYER_GOOGLE_threading"
                   << "VK_LAYER_LUNARG_parameter_validation"
                   << "VK_LAYER_LUNARG_object_tracker"
                   << "VK_LAYER_LUNARG_standard_validation"
                   << "VK_LAYER_LUNARG_image"
                   << "VK_LAYER_LUNARG_swapchain"
                   << "VK_LAYER_GOOGLE_unique_objects");

    inst.setExtensions(QByteArrayList()
                       << "VK_KHR_get_physical_device_properties2");

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    setVulkanInstance(&inst);

    volkLoadInstance(inst.vkInstance());

    resize(size);
    
    QCursor c = cursor();
    c.setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    setCursor(c);

}

Windu::~Windu() {
    device.logical.waitIdle();
    delete vkip;
    delete vkdp;
}

void Windu::start() {

    swap.getSurface();

    if(!loaded) {

        vki = inst.functions();
        vkip = new vk::DispatchLoaderDynamic(static_cast<vk::Instance> (inst.vkInstance()));
        device.init();
        vkd = inst.deviceFunctions(static_cast<VkDevice> (device.logical));
        
        prepareGraph();
    }

    swap.init();

    if(!loaded) {
        
        // Specify resources to be allocated
        renderer.preinit();
        transfer.preinit();

        objects.preinit();
        
        // Allocate memory
        resman.init();
        
        objects.init();
        
        camera.init(swap.extent.width, swap.extent.height);
        
        renderer.init();
        transfer.init();
        
        sync.init();
        loaded = true;
        timer.start();
        
    } else {
        camera.reset(swap.extent.width, swap.extent.height);
        renderer.reset();
    }
    
    wasResized = false;
    requestUpdate();
}

void Windu::reset() {
    
}

void Windu::prepareGraph() {

    swap.signalTo(&renderer, vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    renderer.signalTo(&swap, vk::PipelineStageFlagBits::eTopOfPipe);

}

void Windu::render() {
    if(destroying) {
        emit destroy();
        return;
    }

    if(wasResized) {
        device.logical.waitIdle();
        start();
        return;
    }

    pause.lock();

    qint64 currentNano = timer.nsecsElapsed();
    
    float delta = (currentNano - lastNano)/1000000.;
    camera.step(delta);
    lastNano = currentNano;

    time++;
    if(time>=100 && Settings::is("logging/PrintFPS")) {
        double frametime = (currentNano - lastHun)/time/1000000.;
        qInfo() << frametime << "ms since last frame. fps:" << 1000./frametime;
        lastHun = currentNano;
        time = 0;
    }
    
    ui->update(delta);
    
    if(wasClicked) {
        model->click(renderer.highlight_pos);
        wasClicked = false;
    }

    i = swap.acquire();
    
    transfer.render(i);
    
    model->tick(delta);
    
    renderer.render(i);

    swap.present();
    
    sync.step();
    
    pause.unlock();

    requestUpdate();
}

float Windu::getTime() {
    return timer.nsecsElapsed() / 1000000.;
}

void Windu::update() {
    renderer.clickX = renderer.highlightX;
    renderer.clickY = renderer.highlightY;
}

void Windu::exposeEvent(QExposeEvent *) {
    if (isExposed() && !loaded) {
        start();
    }
}

void Windu::resizeEvent(QResizeEvent *ev) {
    size = ev->size();
    wasResized = true;
}

void Windu::keyPressEvent(QKeyEvent *ev) {
    if(ev->key() == Settings::getKey(Settings::Exit)) {
        destroying = true;
    }

    if(ev->key() == Settings::getKey(Settings::Menu)) {
        ui->menu = !ui->menu;
    }

    camera.keyPressEvent(ev);
}

void Windu::keyReleaseEvent(QKeyEvent *ev) {
    camera.keyReleaseEvent(ev);
}

void Windu::mouseMoveEvent(QMouseEvent *ev) {
    camera.mouseMoveEvent(ev);
}

void Windu::mousePressEvent(QMouseEvent* ev) {
    wasClicked = true;
    isClicked = true;
}

void Windu::mouseReleaseEvent(QMouseEvent* ev) {
    isClicked = false;
}

bool Windu::event(QEvent *e) {
    switch (e->type()) {
        case QEvent::UpdateRequest:
            render();
            break;
        // The swapchain must be destroyed before the surface as per spec. This is
        // not ideal for us because the surface is managed by the QPlatformWindow
        // which may be gone already when the unexpose comes, making the validation
        // layer scream. The solution is to listen to the PlatformSurface events.
        case QEvent::PlatformSurface:
            if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                if(Settings::is("logging/PrintResize")) qInfo("Swapchain resetting");
                swap.reset();
            }
            break;
        default:
            break;
    }
    return QWindow::event(e);
}

void Windu::setRenderSystem(RenderSystem* render) {
    model = render;
}

void Windu::setUISystem(UISystem *ui) {
    this->ui = ui;
}
