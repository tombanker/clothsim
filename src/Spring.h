#pragma once

/// @file Spring.h
/// Represents spring connections between particles in the mass-spring model.

/// Spring type determines physical behavior and connections.
enum class SpringType
{
    /// Structural springs connect immediate neighbors (up, down, left, right).
    /// These are the primary springs defining cloth shape.
    /// Typically 1600-2000 for 30×30, 4000+ for 50×50
    /// Stiffness: springStiffness (e.g., 500)
    Structural,

    /// Shear springs connect diagonal neighbors (NE, NW, SE, SW).
    /// Prevent the quad from collapsing into a triangle (face stability).
    /// Count: same as structural (one diagonal per quad)
    /// Stiffness: springStiffness (same as structural)
    Shear,

    /// Bending springs connect two-ring neighbors (skip one particle).
    /// Resist bending/wrinkling, control cloth curvature.
    /// Count: roughly half of structural (two directions per row/column)
    /// Stiffness: bendStiffness (typically 1/10 of structural, e.g., 50)
    Bending
};

/// Spring connecting two particles.
/// Applies Hooke's Law force + damping to resist stretching/compression.
///
/// **Spring Force (Hooke's Law):**
/// F = -k * (dist - restLength) * direction + damping
///
/// where:
/// - k = stiffness (higher = stiffer, resists length change more)
/// - dist = current distance between particles
/// - restLength = natural resting length
/// - direction = normalized vector from p_a to p_b
/// - damping = velocity-dependent force to reduce oscillation
///
/// **Stiffness ranges (tuned for cloth feel):**
/// - Structural/Shear: 300-2000 (typically 500)
/// - Bending: 10-500 (typically 50, about 1/10 of structural)
/// - Higher = stiffer cloth (less stretch), more expensive computationally
/// - Lower = stretchier cloth, more realistic draping
///
/// **Damping (Rayleigh):**
/// - Applied along spring direction only (prevents over-damping)
/// - Formula: F_damp = -d * (v_rel · spring_dir) * spring_dir
/// - Range: 0.01 - 1.0 (typically 0.1)
/// - Higher = less oscillation, more stable
/// - Lower = more oscillation, more lively
///
/// **Constraint Satisfaction:**
/// Springs are also constrained to [minLen, maxLen] each frame,
/// where minLen = restLength * maxCompress, maxLen = restLength * maxStretch
struct Spring
{
    int        a, b;        ///< Particle indices: connects particles[a] to particles[b]
    float      restLength;  ///< Natural resting length (computed once from initial positions)
    float      stiffness;   ///< Spring constant k (Hooke's Law)
    float      damping;     ///< Damping coefficient (velocity-dependent force)
    SpringType type;        ///< Type: Structural, Shear, or Bending
};
