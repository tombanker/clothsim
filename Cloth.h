#pragma once

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
    glm::vec3 gravity         = { 0.f, -9.8f, 0.f };
    float     airDamping      = 0.01f;
    float     springStiffness = 500.f;
    float     bendStiffness   = 50.f;
    float     springDamping   = 0.1f;
    float     maxStretch      = 1.10f;
    float     maxCompress     = 0.90f;
    int       constraintIters = 15;

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
