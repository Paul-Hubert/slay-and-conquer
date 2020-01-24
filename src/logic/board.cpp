#include "board.h"

#include "cereal/types/vector.hpp"
#include <cmath>
#include "util/position_converter.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

Board::Board(uint32_t width, uint32_t height) : width(width), height(height), grid(width*height) {
    
}

Board::~Board() {
    
}

Entity Board::get(uint x, uint y) {
    if(x >= width || y >= height) {
        return entt::null;
    }
    return grid[y*width+x];
}

Entity Board::get(PositionComponent comp) {
    if(comp.x >= width || comp.y >= height) {
        return entt::null;
    }
    return grid[comp.y*width+comp.x];
}

void Board::set(uint x, uint y, Entity e) {
    grid[y*width+x] = e;
}

void Board::set(PositionComponent comp, Entity e) {
    grid[comp.y*width+comp.x] = e;
}

std::vector<PositionComponent> Board::neighbours(PositionComponent pos) {
    return {
        PositionComponent(pos.x, pos.y + 1, 0),
        PositionComponent(pos.x + 1, pos.y, 0),
        PositionComponent(pos.x - 1, pos.y + 1, 0),
        PositionComponent(pos.x + 1, pos.y - 1, 0),
        PositionComponent(pos.x - 1, pos.y, 0),
        PositionComponent(pos.x, pos.y - 1, 0)
    };
}

float Board::distance(PositionComponent a, PositionComponent b) {
    return glm::length(PositionConverter::hexToOrtho(a)-PositionConverter::hexToOrtho(b));
}

float Board::distance2(PositionComponent a, PositionComponent b) {
    return glm::length2(PositionConverter::hexToOrtho(a)-PositionConverter::hexToOrtho(b));
}
