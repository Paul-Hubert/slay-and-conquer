#ifndef POSITION_CONVERTER_H
#define POSITION_CONVERTER_H

#include <glm/glm.hpp>

#include "logic/components/position_component.h"

namespace PositionConverter {
    PositionComponent orthoToHex(glm::vec3 in);

    glm::vec3 hexToOrtho(PositionComponent pos);
}
#endif
