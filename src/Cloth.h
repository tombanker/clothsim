#pragma once

#include "Constants.h"
#include "Particle.h"
#include "Spring.h"

#include <glm/glm.hpp>
#include <vector>

class Cloth
{
public:
    Cloth(int rows, int cols, float spacing);

    // Simulation
    void update(float dt);
    void reset();

    // Pin control
    void pin(int row, int col);
    void unpinAll();

    // Collision
    void handleSphereCollision(glm::vec3 center, float radius);
    void handleSelfCollisions();

    // Accessors (for renderer)
    const std::vector<Particle>& getParticles() const { return particles; }
    const std::vector<Spring>&   getSprings()   const { return springs;   }
    int getRows() const { return rows; }
    int getCols() const { return cols; }

    // Simulation parameters (public so ImGui can write to them directly)
    glm::vec3 gravity         = DEFAULT_GRAVITY;
    float     airDamping      = DEFAULT_AIR_DAMPING;
    float     springStiffness = DEFAULT_SPRING_STIFFNESS;
    float     bendStiffness   = DEFAULT_BEND_STIFFNESS;
    float     springDamping   = DEFAULT_SPRING_DAMPING;
    float     maxStretch      = DEFAULT_MAX_STRETCH;
    float     maxCompress     = DEFAULT_MAX_COMPRESS;
    int       constraintIters = DEFAULT_CONSTRAINT_ITERS;
    // Wind
    bool      windEnabled     = false;
    float     windStrength    = DEFAULT_WIND_STRENGTH;
    glm::vec3 windDirection   = DEFAULT_WIND_DIRECTION;
    float     globalTime      = 0.f;  // accumulated time for wind animation

private:
    // Helper: flat index from (row, col)
    int idx(int row, int col) const { return row * cols + col; }

    // Build helpers
    void buildParticles();
    void buildSprings();
    void addSpring(int a, int b, float stiffness, SpringType type);

    // Physics steps (called by update)
    void applyForces();
    void integrate(float dt);
    void satisfyConstraints();

    std::vector<Particle> particles;
    std::vector<Spring>   springs;

    int   rows, cols;
    float spacing;
};
