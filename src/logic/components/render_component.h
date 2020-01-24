#ifndef RENDER_COMPONENT_H
#define RENDER_COMPONENT_H

#include <glm/glm.hpp>

#include <QtGlobal>

class RenderComponent final {
public:
    enum Mesh {unit = 0, castle = 1, arrow = 2, hexagon = 3, water = 4};
    const static uint meshCount = water+1;
    
    RenderComponent(Mesh mesh = unit) {
        this->mesh = mesh;
    }
    
    Mesh mesh;
    uint index = 0;
    glm::vec3 pos;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    {
        ar(mesh);
    }
};

class RotationYRenderComponent final {
public:
    
    float angle;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    {
        ar(angle);
    }
    
};

class RotationRenderComponent final {
public:
    
    float angleX;
    float angleZ;
    
    template<class Archive> 
    void serialize(Archive & ar) 
    {
        ar(angleX, angleZ);
    }
    
};

#endif // RENDER_COMPONENT_H
