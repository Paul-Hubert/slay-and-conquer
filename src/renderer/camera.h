#ifndef CAMERA_H
#define CAMERA_H

#include <QKeyEvent>
#include <QMouseEvent>
#include <qglobal.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();
    void init(int width, int height);
    void setup(int width, int height);
    void cleanup();
    void reset(int width, int height);
    void step(float dt);
    ~Camera();
    glm::mat4 getViewProj();
    glm::mat4 getProj();
    glm::mat4 getView();
    glm::vec3 getViewPos();
    glm::vec3 getCameraPos();
    void setTargetCameraPos(glm::vec3 pos);
    glm::vec3 getTargetCameraPos();
    glm::vec3 getMouseWorldPosition(float depth);
    glm::vec3 getWorldPosition(glm::vec3 screen);
    void mouseMoveEvent(QMouseEvent *ev);
    void keyPressEvent(QKeyEvent *ev);
    void keyReleaseEvent(QKeyEvent *ev);
    
    void poll(QKeyEvent *ev, bool value);
    
    int mouseX = 0, mouseY = 0;
    
private:
    
    glm::vec3 pos, targetPos;
    
    glm::mat4 proj, view;

    double yangle = 0.0;
    
    int width, height, mouseMaxX, mouseMaxY;

    double speed, maxHeight, minHeight;
    bool up = false, down = false, left = false, right = false, turnleft = false, turnright = false, shift = false, space = false;


    bool wasUp = false, wasDown = false, wasRight = false, wasLeft = false;
    void resetHorizontal();
    void resetVertical();

};
#endif
