#pragma once

#include <glm/glm.hpp>

struct Particle
{
    glm::vec3 position;
    glm::vec3 prevPosition;  // for Verlet integration
    glm::vec3 velocity;
    glm::vec3 force;
    float     mass;
    bool      pinned;        // if true, particle ignores all forces
};
