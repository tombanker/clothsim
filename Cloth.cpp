#include "Cloth.h"

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

// ── Constructor ──────────────────────────────────────────────────────────────
Cloth::Cloth(int rows, int cols, float spacing)
    : rows(rows), cols(cols), spacing(spacing)
{
    buildParticles();
    buildSprings();
}

// ── Reset ────────────────────────────────────────────────────────────────────
void Cloth::reset()
{
    particles.clear();
    springs.clear();
    buildParticles();
    buildSprings();
}

// ── Pin helpers ──────────────────────────────────────────────────────────────
void Cloth::pin(int row, int col)
{
    particles[idx(row, col)].pinned = true;
}

void Cloth::unpinAll()
{
    for (auto& p : particles)
        p.pinned = false;
}

// ── Build particles ──────────────────────────────────────────────────────────
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
            p.position     = { startX + c * spacing,
                                startY - r * spacing,
                                0.f };
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

// ── Build springs ────────────────────────────────────────────────────────────
void Cloth::buildSprings()
{
    float diagSpacing   = spacing * std::sqrt(2.f);
    float bendSpacing   = spacing * 2.f;
    float bendDiagSpac  = spacing * std::sqrt(2.f) * 2.f;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            // ── Structural ───────────────────────────────────────────────────
            // right neighbor
            if (c + 1 < cols)
                addSpring(idx(r, c), idx(r, c + 1), springStiffness, SpringType::Structural);
            // down neighbor
            if (r + 1 < rows)
                addSpring(idx(r, c), idx(r + 1, c), springStiffness, SpringType::Structural);

            // ── Shear ────────────────────────────────────────────────────────
            // diagonal down-right
            if (r + 1 < rows && c + 1 < cols)
                addSpring(idx(r, c), idx(r + 1, c + 1), springStiffness, SpringType::Shear);
            // diagonal down-left
            if (r + 1 < rows && c - 1 >= 0)
                addSpring(idx(r, c), idx(r + 1, c - 1), springStiffness, SpringType::Shear);

            // ── Bending ──────────────────────────────────────────────────────
            // two-ring right
            if (c + 2 < cols)
                addSpring(idx(r, c), idx(r, c + 2), bendStiffness, SpringType::Bending);
            // two-ring down
            if (r + 2 < rows)
                addSpring(idx(r, c), idx(r + 2, c), bendStiffness, SpringType::Bending);
        }
    }
}

// ── addSpring helper ─────────────────────────────────────────────────────────
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

// ── Update (called once per frame) ───────────────────────────────────────────
void Cloth::update(float dt)
{
    applyForces();
    integrate(dt);
    satisfyConstraints();
}

// ── Force accumulation ───────────────────────────────────────────────────────
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

        // Hooke's Law
        glm::vec3 springF   = s.stiffness * stretch * dir;

        // Spring damping along the spring axis
        glm::vec3 relVel    = pb.velocity - pa.velocity;
        glm::vec3 dampF     = s.damping * glm::dot(relVel, dir) * dir;

        glm::vec3 totalF    = springF + dampF;

        if (!pa.pinned) pa.force += totalF;
        if (!pb.pinned) pb.force -= totalF;
    }
}

// ── Verlet integration ───────────────────────────────────────────────────────
void Cloth::integrate(float dt)
{
    for (auto& p : particles)
    {
        if (p.pinned) continue;

        glm::vec3 accel    = p.force / p.mass;
        glm::vec3 newPos   = 2.f * p.position - p.prevPosition + accel * dt * dt;

        // Recover velocity for damping next frame
        p.velocity     = (newPos - p.prevPosition) / (2.f * dt);
        p.prevPosition = p.position;
        p.position     = newPos;
    }
}

// ── Constraint satisfaction (max stretch) ────────────────────────────────────
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

            float minLen = s.restLength * maxCompress;
            float maxLen = s.restLength * maxStretch;

            if (dist < minLen || dist > maxLen)
            {
                float     target    = glm::clamp(dist, minLen, maxLen);
                glm::vec3 correction = delta * ((dist - target) / dist);

                // Split correction between the two particles
                // A pinned particle absorbs none — the other takes all
                if      (!pa.pinned && !pb.pinned) {
                    pa.position += correction * 0.5f;
                    pb.position -= correction * 0.5f;
                }
                else if (!pa.pinned) pa.position += correction;
                else if (!pb.pinned) pb.position -= correction;
            }
        }
    }
}

// ── Sphere collision ─────────────────────────────────────────────────────────
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

// ── Self collision (marble algorithm) ────────────────────────────────────────
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
                glm::vec3 correction = delta * ((dist - minDist) / dist);
                if (!pa.pinned) pa.position += correction * 0.5f;
                if (!pb.pinned) pb.position -= correction * 0.5f;
                // Zero out velocity components toward each other
                pa.velocity = { 0.f, 0.f, 0.f };
                pb.velocity = { 0.f, 0.f, 0.f };
            }
        }
    }
}
