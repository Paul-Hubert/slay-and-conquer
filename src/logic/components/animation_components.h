#ifndef ANIMATION_COMPONENTS_H
#define ANIMATION_COMPONENTS_H

#include <glm/glm.hpp>
#include <functional>

class PathAnimationComponent {
public:
    std::vector<glm::vec3> positions;
    unsigned int index;
    float t;
    std::function<void()> callback;
    PathAnimationComponent(std::vector<glm::vec3> positions, std::function<void()> callback) : positions(positions), index(0), t(0.f), callback(callback) {}
    PathAnimationComponent(std::vector<glm::vec3> positions) : positions(positions), index(0), t(0.f), callback(nullptr) {}
};

class NoAnimationComponent {};

class ParabolaAnimationComponent {
public:
    glm::vec3 start;
    glm::vec3 end;
    float t = 0;
    std::function<void()> callback;
    ParabolaAnimationComponent(glm::vec3 start, glm::vec3 end, std::function<void()> callback) : start(start), end(end), t(0.f), callback(callback) {}
    ParabolaAnimationComponent(glm::vec3 start, glm::vec3 end) : start(start), end(end), t(0.f), callback(nullptr) {}
};

#endif
