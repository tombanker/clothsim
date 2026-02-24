#pragma once

#include "Constants.h"
#include "Particle.h"
#include "Spring.h"

#include <glm/glm.hpp>
#include <vector>

/// @file Cloth.h
/// Cloth simulation using mass-spring model with Verlet integration.
///
/// **Physics Model:**
/// - Particles are connected by springs (structural, shear, bending)
/// - Forces: gravity, spring tension, damping, air resistance, wind
/// - Integration: Verlet (4th-order accurate, unconditionally stable)
/// - Constraints: max-stretch enforcement via iterative constraint solving
///
/// **Update Loop (per frame):**
/// 1. applyForces() — accumulate gravity, spring forces, damping, wind
/// 2. integrate()  — update positions via Verlet, recover velocities
/// 3. satisfyConstraints() — enforce spring length limits (prevents explosion)
///
/// **Key Reference:**
/// Matt Fisher's Cloth Tutorial: https://graphics.stanford.edu/~mdfisher/cloth.html

class Cloth
{
public:
    /// Constructor: create cloth grid with given dimensions.
    /// Generates particles in a grid and connects them with springs.
    ///
    /// @param rows Number of particles vertically
    /// @param cols Number of particles horizontally
    /// @param spacing Distance between adjacent particles (meters)
    ///
    /// Layout: cloth hangs in XZ plane, Y points up. Top two corners are pinned.
    Cloth(int rows, int cols, float spacing);

    /// Reset cloth to initial state (recreate particles and springs).
    void reset();

    /// Pin a particle at (row, col) — it will not move during simulation.
    void pin(int row, int col);

    /// Unpin all particles — allow all to move freely.
    void unpinAll();

    /// Main simulation step — called once per frame.
    /// Executes: applyForces → integrate → satisfyConstraints
    void update(float dt);

    /// Handle collision between cloth and a sphere.
    /// Projects any particle inside the sphere to its surface (distance = radius).
    void handleSphereCollision(glm::vec3 center, float radius);

    /// Handle self-collisions using marble algorithm.
    /// Treats each particle as a sphere, prevents interpenetration.
    void handleSelfCollisions();

    // Accessors (for renderer)
    const std::vector<Particle>& getParticles() const { return particles; }
    const std::vector<Spring>&   getSprings()   const { return springs;   }
    int getRows() const { return rows; }
    int getCols() const { return cols; }

    /// **Simulation Parameters** (public for real-time ImGui adjustment)
    /// All can be tuned at runtime without rebuild.

    /// Gravitational acceleration (m/s²). Typical: {0, -9.8, 0}
    glm::vec3 gravity         = DEFAULT_GRAVITY;

    /// Air resistance coefficient. Higher = more drag. Range: [0, 0.5]
    float     airDamping      = DEFAULT_AIR_DAMPING;

    /// Spring stiffness for structural and shear springs. Range: [1, 2000]
    /// Higher = stiffer cloth, more resistance to stretching
    float     springStiffness = DEFAULT_SPRING_STIFFNESS;

    /// Bending spring stiffness. Range: [0, 500]
    /// Controls resistance to bending/wrinkling. Typically 1/10 of springStiffness
    float     bendStiffness   = DEFAULT_BEND_STIFFNESS;

    /// Spring damping coefficient (Rayleigh damping). Range: [0, 1]
    /// Smooths motion, reduces oscillation. Critical for stability.
    /// Applied as: F_damp = -damping * (v_rel · spring_dir) * spring_dir
    float     springDamping   = DEFAULT_SPRING_DAMPING;

    /// Maximum stretch factor. Range: [1.0, 1.3]
    /// If spring_dist > maxStretch * rest_length, constraint clamps it down.
    /// Prevents unrealistic stretching
    float     maxStretch      = DEFAULT_MAX_STRETCH;

    /// Maximum compression factor. Range: [0.7, 1.0]
    /// If spring_dist < maxCompress * rest_length, constraint expands it.
    /// Prevents collapse
    float     maxCompress     = DEFAULT_MAX_COMPRESS;

    /// Number of constraint solver iterations per frame. Range: [1, 40]
    /// Higher = more accurate but slower.
    /// Each iteration processes all springs once (O(springs) per iter).
    /// Typical: 8-15. Use lower values for real-time performance.
    int       constraintIters = DEFAULT_CONSTRAINT_ITERS;

    /// Wind force parameters
    bool      windEnabled     = false;       ///< Enable/disable wind
    float     windStrength    = DEFAULT_WIND_STRENGTH;  ///< Wind magnitude
    glm::vec3 windDirection   = DEFAULT_WIND_DIRECTION; ///< Wind direction (normalized)
    float     globalTime      = 0.f;         ///< Accumulated time for wind oscillation

private:
    /// Helper: compute flat index from (row, col) grid coordinates.
    /// Particles stored as: particles[row * cols + col]
    /// Used throughout: particle[idx(r, c)] = particle at grid(r, c)
    int idx(int row, int col) const { return row * cols + col; }

    /// Build initial particle grid. Called by constructor and reset().
    /// Places particles in a regular grid, hangs from top two corners.
    void buildParticles();

    /// Build spring network. Called by constructor and reset().
    /// Connects particles with:
    /// - Structural springs (immediate neighbors: up, down, left, right)
    /// - Shear springs (diagonals: prevents face collapse)
    /// - Bending springs (two-ring: resists wrinkling)
    void buildSprings();

    /// Add a spring between particles a and b.
    /// Sets rest length to current distance, stores stiffness and type.
    void addSpring(int a, int b, float stiffness, SpringType type);

    /// **Physics Step 1: Force Accumulation**
    /// Compute total force acting on each particle:
    ///
    /// **Gravity:** F = m * g (applied to all particles)
    ///
    /// **Spring Forces (Hooke's Law + Damping):**
    /// - Spring force: F_spring = -k * (|x| - L0) * x_hat
    ///   where x = p_b - p_a, L0 = rest length, x_hat = normalized direction
    /// - Damping: F_damp = -d * (v_rel · x_hat) * x_hat
    ///   where v_rel = relative velocity along spring
    /// - Total on p_a: F = F_spring + F_damp
    /// - Total on p_b: F = -F (Newton's 3rd law)
    ///
    /// **Air Resistance:** F_air = -airDamp * velocity
    ///   Simulates air friction, smooths jitter
    ///
    /// **Wind:** F_wind = windStrength * sin(globalTime) * windDirection * mass
    ///   Oscillating force, animates cloth in breeze
    void applyForces();

    /// **Physics Step 2: Verlet Integration**
    /// Updates particle positions using Verlet method (4th-order accurate).
    ///
    /// **Standard Explicit Euler (1st-order):**
    ///   v(t+dt) = v(t) + a(t)*dt
    ///   x(t+dt) = x(t) + v(t)*dt
    /// Problem: 1st-order accuracy, unstable for large dt
    ///
    /// **Verlet Method (4th-order):**
    ///   x(t+dt) = 2*x(t) - x(t-dt) + a(t)*dt²
    ///
    /// Benefits:
    /// - 4th-order accuracy (vs 1st-order Euler)
    /// - Unconditionally stable for small dt
    /// - No explicit velocity storage needed
    /// - Automatic momentum conservation
    ///
    /// **Process:**
    /// 1. Compute acceleration: a = F / m
    /// 2. Update position: x_new = 2*x - x_prev + a*dt²
    /// 3. Recover velocity (for damping): v = (x_new - x_prev) / (2*dt)
    /// 4. Advance state: x_prev = x, x = x_new
    ///
    /// **Pinned particles:** not updated (position stays fixed)
    void integrate(float dt);

    /// **Physics Step 3: Constraint Satisfaction**
    /// Enforce spring length constraints iteratively.
    ///
    /// **Problem:**
    /// Forces alone can cause springs to stretch/compress excessively,
    /// leading to instability and "jelly cloth" artifacts.
    ///
    /// **Solution:**
    /// Iteratively clamp spring lengths to [minLen, maxLen] range.
    /// This is the KEY STABILITY MECHANISM (more important than time step size).
    ///
    /// **Algorithm (per iteration):**
    /// ```
    /// for each spring s:
    ///   delta = p_b - p_a
    ///   dist = |delta|
    ///   if dist < minLen or dist > maxLen:
    ///     target = clamp(dist, minLen, maxLen)
    ///     correction = delta * (dist - target) / dist
    ///     if both unpinned:   split correction 50/50
    ///     if p_a pinned:      p_b absorbs full correction
    ///     if p_b pinned:      p_a absorbs full correction
    /// ```
    ///
    /// **Why iterate?**
    /// Fixing one spring can violate neighbors. Multiple passes let
    /// corrections propagate through the network.
    /// Typical: 8-15 iterations for smooth, stable cloth.
    void satisfyConstraints();

    std::vector<Particle> particles;  ///< Grid of particles (flat 1D array)
    std::vector<Spring>   springs;    ///< All springs connecting particles

    int   rows, cols;  ///< Grid dimensions
    float spacing;     ///< Distance between adjacent particles
};

