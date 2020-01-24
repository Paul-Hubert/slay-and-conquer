#include "camera.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <QDebug>
#include <QCursor>

#include "util/settings.h"

Camera::Camera() {
    pos = glm::vec3(0, 16, 0);
    targetPos = pos;
    maxHeight = 25;
    minHeight = 7;
}

void Camera::init(int width, int height) {
    speed = Settings::cameraSpeed();
    setup(width, height);
}

void Camera::setup(int width, int height) {
    this->width = width;
    this->height = height;
    proj = glm::perspective(glm::radians(70.0f), width / (float) height, 0.1f, 1000.0f);
    proj[1][1] = -proj[1][1];
    mouseX = 100;
    mouseY = 100;
}

void Camera::cleanup() {
    
}

void Camera::reset(int width, int height) {
    cleanup();
    setup(width, height);
}

Camera::~Camera() {
    
}

void Camera::step(float dt) {
    
    float speed = this->speed * ((pos.y-minHeight) / (maxHeight-minHeight) + 0.8);

    glm::vec3 lastPos = pos;
    
    if(left) {
        pos.x += speed*sin(M_PI/2.0 + yangle)*dt;
        pos.z += speed*cos(M_PI/2.0 + yangle)*dt;
    } if(right) {
        pos.x -= speed*sin(M_PI/2.0 + yangle)*dt;
        pos.z -= speed*cos(M_PI/2.0 + yangle)*dt;
    } if(up) {
        pos.x += speed*sin(yangle)*dt;
        pos.z += speed*cos(yangle)*dt;
    } if(down) {
        pos.x -= speed*sin(yangle)*dt;
        pos.z -= speed*cos(yangle)*dt;
    }
    
    if(Settings::is("input/camera/MouseMoveCamera")) {
        if(mouseX < 30) { //Gauche
            pos.x += speed*sin(M_PI/2.0 + yangle)*dt;
            pos.z += speed*cos(M_PI/2.0 + yangle)*dt;
        }
        if(mouseX > width-30) { //Droite
            pos.x -= speed*sin(M_PI/2.0 + yangle)*dt;
            pos.z -= speed*cos(M_PI/2.0 + yangle)*dt;
        }
        if(mouseY < 30) { //Haut
            pos.x += speed*sin(yangle)*dt;
            pos.z += speed*cos(yangle)*dt;
        }
        if(mouseY > height-30) { //Bas
            pos.x -= speed*sin(yangle)*dt;
            pos.z -= speed*cos(yangle)*dt;
        }
    }

    if(space) pos.y += this->speed*dt;
    if(shift) pos.y -= this->speed*dt;
    
    if(space || shift) {
        pos.y = fmin(fmax(pos.y, minHeight), maxHeight);
    }
    
    if(Settings::is("input/camera/MouseRotateCamera")) {
        if(mouseX < 30) {
            yangle += 30 * (M_PI*0.1/180.) * dt/16.f;
        } else if(mouseX > width - 30) {
            yangle -= 30 * (M_PI*0.1/180.) * dt/16.f;
        }
    }
    
    if(turnleft) {
        yangle += 30 * (M_PI*0.1/180.) * dt/16.f;
    } else if(turnright) {
        yangle -= 30 * (M_PI*0.1/180.) * dt/16.f;
    }

    if(lastPos != pos) {
        targetPos = pos;
    } else {
        pos += (targetPos - pos) * 0.1f * dt/16.f;
    }
    
    view = glm::lookAt(getCameraPos(), getViewPos(), glm::vec3(0,1,0));
}

glm::mat4 Camera::getViewProj() {
    return proj * getView();
}

glm::mat4 Camera::getProj() {
    return proj;
}

glm::mat4 Camera::getView() {
    return view;
}

glm::vec3 Camera::getViewPos() {
    return glm::vec3(-pos.x, -6, -pos.z);
}

void Camera::setTargetCameraPos(glm::vec3 pos) {
    targetPos = glm::vec3(-pos.x, this->pos.y, -pos.z);
}

glm::vec3 Camera::getTargetCameraPos() {
    return glm::vec3(-targetPos.x, pos.y, -targetPos.z);
}

glm::vec3 Camera::getCameraPos() {
    const float dist = 10;
    return glm::vec3(-pos.x + dist*sin(yangle), pos.y, -pos.z + dist*cos(yangle));
}

glm::vec3 Camera::getMouseWorldPosition(float depth) {
    return glm::unProject(glm::vec3(mouseX, mouseY, depth), view, proj, glm::vec4(0, 0, width, height));
}

glm::vec3 Camera::getWorldPosition(glm::vec3 screen) {
    return glm::unProject(screen, view, proj, glm::vec4(0, 0, width, height));
}

void Camera::mouseMoveEvent(QMouseEvent *ev) {
    mouseX = ev->pos().x();
    mouseY = ev->pos().y();
    
    if(Settings::is("fix/input/MouseHack")) {
        if(mouseX > mouseMaxX) mouseMaxX = mouseX;
        if(mouseY > mouseMaxY) mouseMaxY = mouseY;
        width = mouseMaxX;
        height = mouseMaxY;
    }
}

void Camera::keyPressEvent(QKeyEvent *ev) {
    poll(ev, true);
}

void Camera::keyReleaseEvent(QKeyEvent *ev) {
    poll(ev, false);
}

void Camera::poll(QKeyEvent *ev, bool value) {
    if(ev->key() == Settings::getKey(Settings::Forward)) {
        this->up = value;
    } else if(ev->key() == Settings::getKey(Settings::Backward)) {
        this->down = value;
    } else if(ev->key() == Settings::getKey(Settings::RightStrafe)) {
        this->right = value;
    } else if(ev->key() == Settings::getKey(Settings::Right)) {
        this->turnright = value;
    } else if(ev->key() == Settings::getKey(Settings::LeftStrafe)) {
        this->left = value;
    } else if(ev->key() == Settings::getKey(Settings::Left)) {
        this->turnleft = value;
    } else if(ev->key() == Settings::getKey(Settings::Up)) {
        this->space = value;
    } else if(ev->key() == Settings::getKey(Settings::Down)) {
        this->shift = value;
    }
}

void Camera::resetHorizontal() {
    wasRight = false;
    wasLeft = false;
}

void Camera::resetVertical() {
    wasUp = false;
    wasDown = false;
}
