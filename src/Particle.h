#pragma once

#include <glm/glm.hpp>

/// @file Particle.h
/// Represents a single mass node in the cloth simulation.
/// Particles are connected by springs and updated via Verlet integration.

/// Single particle in the cloth grid.
///
/// **State Variables:**
/// - position: current world position (meters)
/// - prevPosition: position from last frame (used for Verlet integration)
/// - velocity: current velocity (m/s) — recovered from position difference
/// - force: accumulated forces from gravity, springs, wind, etc.
/// - mass: particle mass (kg), typically 1.0
/// - pinned: if true, position is fixed (immovable anchor point)
///
/// **Usage:**
/// Particles are stored in a flat 1D array in Cloth::particles.
/// Access as: particles[row * cols + col] for grid(row, col)
///
/// **Verlet Integration:**
/// Each frame:
/// 1. applyForces() computes particle.force
/// 2. integrate() updates position using: x_new = 2*x - x_prev + (F/m)*dt²
/// 3. Velocity is recovered as: v = (x_new - x_prev) / (2*dt)
/// 4. satisfyConstraints() clamps spring lengths
///
/// **Pinning:**
/// If pinned=true:
/// - Position never changes during update()
/// - No forces applied
/// - Used for fixed cloth boundaries or attachment points
struct Particle
{
    glm::vec3 position;      ///< Current world position (meters)
    glm::vec3 prevPosition;  ///< Previous frame position (for Verlet integration)
    glm::vec3 velocity;      ///< Current velocity (m/s), recovered from positions
    glm::vec3 force;         ///< Accumulated force this frame (for next integration)
    float     mass;          ///< Particle mass (kg)
    bool      pinned;        ///< If true, particle is immovable (fixed constraint)
};
