#include "position_converter.h"

#define _USE_MATH_DEFINES
#include <cmath>

PositionComponent PositionConverter::orthoToHex(glm::vec3 in) {
    float x = floor(in.z/3 + 0.5);
    return PositionComponent(x, floor((in.x/1.732051-x)/2 + 0.5), in.y);
}

glm::vec3 PositionConverter::hexToOrtho(PositionComponent pos) {
    return glm::vec3(1.732051*(2*pos.y+pos.x), pos.height, 3*pos.x);
}
