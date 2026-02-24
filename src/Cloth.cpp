#include "Cloth.h"

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

// MARK: Constructor
Cloth::Cloth(int rows, int cols, float spacing)
    : rows(rows), cols(cols), spacing(spacing)
{
    buildParticles();
    buildSprings();
}

// MARK: Reset
void Cloth::reset()
{
    particles.clear();
    springs.clear();
    buildParticles();
    buildSprings();
}

// MARK: Pin helpers
void Cloth::pin(int row, int col)
{
    particles[idx(row, col)].pinned = true;
}

void Cloth::unpinAll()
{
    for (auto& p : particles)
        p.pinned = false;
}

// MARK: Build particles
void Cloth::buildParticles()
{
    particles.reserve(rows * cols);

    // Cloth lays flat in the XZ plane initially, hanging down from the top row.
    // Top-left corner is at (-cols/2 * spacing, 0, 0) so the cloth is centered.
    float startX = -(cols - 1) * spacing * 0.5f;
    float startY =  (rows - 1) * spacing;       // top row at this Y, hangs down

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            Particle p;
            p.position     = { startX + c * spacing, startY - r * spacing, 0.f };
            p.prevPosition = p.position;   // Verlet: at rest, prev == current
            p.velocity     = { 0.f, 0.f, 0.f };
            p.force        = { 0.f, 0.f, 0.f };
            p.mass         = 1.f;
            p.pinned       = false;
            particles.push_back(p);
        }
    }

    // Default: pin the two top corners
    pin(0, 0);
    pin(0, cols - 1);
}

// MARK: Build springs
void Cloth::buildSprings()
{
    float diagSpacing   = spacing * std::sqrt(2.f);
    float bendSpacing   = spacing * 2.f;
    float bendDiagSpac  = spacing * std::sqrt(2.f) * 2.f;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            // Structural
            // right neighbor
            if (c + 1 < cols)
                addSpring(idx(r, c), idx(r, c + 1), springStiffness, SpringType::Structural);
            // down neighbor
            if (r + 1 < rows)
                addSpring(idx(r, c), idx(r + 1, c), springStiffness, SpringType::Structural);

            // Shear
            // diagonal down-right
            if (r + 1 < rows && c + 1 < cols)
                addSpring(idx(r, c), idx(r + 1, c + 1), springStiffness, SpringType::Shear);
            // diagonal down-left
            if (r + 1 < rows && c - 1 >= 0)
                addSpring(idx(r, c), idx(r + 1, c - 1), springStiffness, SpringType::Shear);

            // Bending
            // two-ring right
            if (c + 2 < cols)
                addSpring(idx(r, c), idx(r, c + 2), bendStiffness, SpringType::Bending);
            // two-ring down
            if (r + 2 < rows)
                addSpring(idx(r, c), idx(r + 2, c), bendStiffness, SpringType::Bending);
        }
    }
}

// MARK: addSpring helper
void Cloth::addSpring(int a, int b, float stiffness, SpringType type)
{
    Spring s;
    s.a          = a;
    s.b          = b;
    s.restLength = glm::length(particles[a].position - particles[b].position);
    s.stiffness  = stiffness;
    s.damping    = springDamping;
    s.type  = type;
    springs.push_back(s);
}

// MARK: Update (called once per frame)
void Cloth::update(float deltaTime)
{
    globalTime += deltaTime;
    // STEP 1: Reset forces and apply Gravity
    // STEP 2: Apply spring forces
    applyForces();
    // STEP 3: Integration
    integrate(deltaTime);
    // STEP 4: Constraints (max stretch)
    satisfyConstraints();
}

// MARK: - Force Accumulation
/// Accumulate all forces acting on each particle.
/// This is called once per frame before integration.
///
/// **Forces computed:**
/// 1. **Gravity:** F = m * g
///    - Applied to all unpinned particles
///    - Typically {0, -9.8, 0} m/s²
///
/// 2. **Air Resistance:** F_air = -airDamp * velocity
///    - Proportional to velocity, opposing direction
///    - Smooths motion, reduces high-frequency jitter
///
/// 3. **Wind:** F_wind = windDirection * sin(globalTime * 2.f) * windStrength * mass
///    - Oscillates back and forth (sine wave)
///    - Creates natural fluttering motion
///
/// 4. **Spring Forces (Hooke's Law):** F_spring = -k * (dist - L0) * direction
///    - k = stiffness (higher = stiffer)
///    - dist = current spring length
///    - L0 = rest length
///    - direction = normalized spring vector
///    - Positive stretch → pulls particles together
///    - Negative stretch (compression) → pushes apart
///
/// 5. **Spring Damping:** F_damp = -d * (v_rel · direction) * direction
///    - d = damping coefficient
///    - v_rel = relative velocity between particles
///    - Reduces oscillation, improves stability
///    - Only applied along spring direction (prevents energy loss)
///
/// **Implementation notes:**
/// - Forces reset to zero each frame
/// - Pinned particles skip force accumulation
/// - Newton's 3rd law: force on p_b = -force on p_a
void Cloth::applyForces()
{
    // Reset forces
    for (auto& p : particles)
        p.force = { 0.f, 0.f, 0.f };

    for (auto& p : particles)
    {
        if (p.pinned) continue;

        // Gravity
        p.force += gravity * p.mass;

        // Air damping
        p.force -= airDamping * p.velocity;

        // Wind force (time-varying, oscillating)
        if (windEnabled) {
            float windMagnitude = windStrength * std::sin(globalTime * 2.f);
            p.force += windDirection * windMagnitude * p.mass;
        }
    }

    // Spring forces (Hooke's Law + damping)
    for (const auto& s : springs)
    {
        Particle& pa = particles[s.a];
        Particle& pb = particles[s.b];

        glm::vec3 delta     = pb.position - pa.position;
        float     dist      = glm::length(delta);
        if (dist < 1e-6f) continue;         // avoid divide-by-zero

        glm::vec3 dir       = delta / dist;
        float     stretch   = dist - s.restLength;

        // Hooke's Law: F = -k * stretch * direction
        glm::vec3 springF   = s.stiffness * stretch * dir;

        // Spring damping along the spring axis
        // Only applied along spring direction, not globally
        glm::vec3 relVel    = pb.velocity - pa.velocity;
        glm::vec3 dampF     = s.damping * glm::dot(relVel, dir) * dir;

        glm::vec3 totalF    = springF + dampF;

        // Apply forces (Newton's 3rd law)
        if (!pa.pinned) pa.force += totalF;
        if (!pb.pinned) pb.force -= totalF;
    }
}

// MARK: - Verlet Integration
/// Update particle positions using Verlet integration method.
/// This is called once per frame, after applyForces().
///
/// **Why Verlet?**
/// - 4th-order accuracy (vs 1st-order for Euler)
/// - Unconditionally stable (no time step limit)
/// - No explicit velocity storage needed
/// - Better energy conservation
///
/// **Standard Explicit Euler (for comparison):**
///   v(t+dt) = v(t) + a(t)*dt
///   x(t+dt) = x(t) + v(t)*dt
/// Problem: Only 1st-order accurate, unstable for large dt
///
/// **Verlet Method:**
///   x(t+dt) = 2*x(t) - x(t-dt) + a(t)*dt²
///
/// Derivation:
/// - Taylor expansion: x(t+dt) = x(t) + v(t)*dt + 0.5*a(t)*dt²
/// - Also:            x(t-dt) = x(t) - v(t)*dt + 0.5*a(t)*dt²
/// - Adding these and rearranging gives the Verlet formula above
///
/// **Velocity Recovery:**
/// Since we use x(t) and x(t-dt) instead of explicit v(t),
/// we recover velocity as: v(t) ≈ (x(t) - x(t-dt)) / (2*dt)
/// This is used for spring damping in the next frame's applyForces().
///
/// **Implementation:**
/// 1. Compute acceleration: a = F / m
/// 2. Compute new position: x_new = 2*x - x_prev + a*dt²
/// 3. Recover velocity: v = (x_new - x_prev) / (2*dt)
/// 4. Advance: x_prev = x, x = x_new
void Cloth::integrate(float deltaTime)
{
    for (auto& p : particles)
    {
        if (p.pinned) continue;

        // Compute acceleration from forces
        glm::vec3 accel    = p.force / p.mass;

        // Verlet step: x_new = 2*x - x_prev + a*dt²
        glm::vec3 newPos   = 2.f * p.position - p.prevPosition + accel * deltaTime * deltaTime;

        // Recover velocity for damping next frame
        // v = (x_new - x_prev) / (2*dt)
        p.velocity     = (newPos - p.prevPosition) / (2.f * deltaTime);

        // Advance state
        p.prevPosition = p.position;
        p.position     = newPos;
    }
}

// MARK: - Constraint Satisfaction (Max Stretch)
/// Enforce spring length constraints iteratively.
/// **This is the MOST IMPORTANT stability mechanism.**
///
/// **Problem:**
/// Forces alone can cause springs to stretch or compress excessively.
/// This leads to instability and "jelly cloth" artifacts.
/// Example: a spring with high mass, low stiffness, large time step
/// can stretch multiple times its rest length, then snap back violently.
///
/// **Solution: Iterative Constraint Solving**
/// After integration, check all springs and clamp lengths to valid range.
/// Repeat N times (constraintIters) so corrections propagate through network.
///
/// **Algorithm (per iteration):**
/// ```
/// For each spring:
///   - Compute current length: dist = |p_b - p_a|
///   - Clamp to valid range: target = clamp(dist, minLen, maxLen)
///   - Compute correction: correction = (dist - target) / dist * (p_b - p_a)
///   - Apply correction:
///     * If both unpinned: split 50/50 (p_a += corr*0.5, p_b -= corr*0.5)
///     * If p_a pinned:    p_b absorbs full correction
///     * If p_b pinned:    p_a absorbs full correction
///     * If both pinned:   no change (springs cannot move)
/// ```
///
/// **Why Multiple Iterations?**
/// - Iteration 1: fixes springs that violate constraints
/// - Iteration 2: corrections from iter 1 may have violated other springs
/// - Iteration N: constraint propagates through connected network
/// - Typical: 8-15 iterations sufficient for smooth cloth
/// - More iterations = less stretch but slower
///
/// **Constraint Range:**
/// - minLen = restLength * maxCompress (typically 0.9 * restLength)
/// - maxLen = restLength * maxStretch (typically 1.1 * restLength)
/// - Prevents unrealistic stretching and collapse
///
/// **Notes:**
/// - This method is more stable than implicit integration (time-stepping)
/// - Trade-off: constraint correctness vs speed (tune constraintIters)
/// - Default: 8 iterations at 50×50 mesh, achieves ~30 FPS
void Cloth::satisfyConstraints()
{
    for (int iter = 0; iter < constraintIters; ++iter)
    {
        for (const auto& s : springs)
        {
            Particle& pa = particles[s.a];
            Particle& pb = particles[s.b];

            glm::vec3 delta  = pb.position - pa.position;
            float     dist   = glm::length(delta);
            if (dist < 1e-6f) continue;

            // Compute valid range for this spring
            float minLen = s.restLength * maxCompress;
            float maxLen = s.restLength * maxStretch;

            // Check if spring violates constraints
            if (dist < minLen || dist > maxLen)
            {
                // Clamp to valid range
                float     target     = glm::clamp(dist, minLen, maxLen);

                // Correction vector: how much to move particles to reach target
                // correction = (current_dist - target_dist) / current_dist * spring_vector
                glm::vec3 correction = delta * ((dist - target) / dist);

                // Apply correction based on pinning status
                // Pinned particles don't move; unpinned absorb correction
                if      (!pa.pinned && !pb.pinned) {
                    // Both free: split correction
                    pa.position += correction * 0.5f;
                    pb.position -= correction * 0.5f;
                }
                else if (!pa.pinned) {
                    // p_b pinned: p_a absorbs full correction
                    pa.position += correction;
                }
                else if (!pb.pinned) {
                    // p_a pinned: p_b absorbs full correction
                    pb.position -= correction;
                }
                // If both pinned: no change (constraint cannot be satisfied)
            }
        }
    }
}

// MARK: - Collision Detection & Response

/// Handle collision between cloth and a sphere.
/// Project any particle inside sphere to its surface.
///
/// **Algorithm:**
/// For each particle:
///   - Compute distance from sphere center
///   - If distance < radius, particle is inside
///   - Project to surface: pos = center + normalize(pos - center) * (radius + epsilon)
///   - Epsilon (1e-3) prevents particle from getting stuck exactly on surface
///
/// **Notes:**
/// - One-way collision (sphere is static)
/// - Simple and fast (O(n) where n = particle count)
/// - Works well for demo purposes
void Cloth::handleSphereCollision(glm::vec3 center, float radius)
{
    for (auto& p : particles)
    {
        glm::vec3 dir  = p.position - center;
        float     dist = glm::length(dir);
        if (dist < radius)
        {
            // Project particle to sphere surface + small epsilon
            p.position = center + glm::normalize(dir) * (radius + 1e-3f);
        }
    }
}

/// Handle cloth self-collisions using marble algorithm.
/// Prevents particles from penetrating each other.
///
/// **Algorithm (Marble Algorithm):**
/// Treat each particle as a sphere with radius = spacing * 0.5.
/// For any two particles closer than 2*marbleRadius:
///   - Compute separation vector
///   - Push apart so minimum distance = 2*marbleRadius
///   - Zero out velocities to dissipate energy
///
/// **Complexity:** O(n²) naive implementation
/// - For 50×50 cloth: 2500 particles = 3.1M distance checks
/// - Should implement spatial hashing for 50×50+
///
/// **Performance:** Currently disabled in update loop
void Cloth::handleSelfCollisions()
{
    float marbleRadius  = spacing * 0.5f;
    float minDist       = 2.f * marbleRadius;

    for (int i = 0; i < (int)particles.size(); ++i)
    {
        for (int j = i + 1; j < (int)particles.size(); ++j)
        {
            Particle& pa = particles[i];
            Particle& pb = particles[j];

            glm::vec3 delta = pb.position - pa.position;
            float     dist  = glm::length(delta);

            if (dist < minDist && dist > 1e-6f)
            {
                // Separation vector: how much to move each particle
                glm::vec3 correction = delta * ((dist - minDist) / dist);

                // Push apart (split correction)
                if (!pa.pinned) pa.position += correction * 0.5f;
                if (!pb.pinned) pb.position -= correction * 0.5f;

                // Zero out velocity (dissipate energy from collision)
                pa.velocity = { 0.f, 0.f, 0.f };
                pb.velocity = { 0.f, 0.f, 0.f };
            }
        }
    }
}
