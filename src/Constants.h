#pragma once

#include <glm/glm.hpp>

// ── Window ───────────────────────────────────────────────────────────────────
constexpr int   WINDOW_WIDTH  = 1280;
constexpr int   WINDOW_HEIGHT = 720;
constexpr char  WINDOW_TITLE[] = "Cloth Simulation";
constexpr char  GLSL_VERSION[] = "#version 330 core";

// ── Cloth grid ───────────────────────────────────────────────────────────────
constexpr int   CLOTH_ROWS    = 40;
constexpr int   CLOTH_COLS    = 40;
constexpr float CLOTH_SPACING = 0.1f;

// ── Physics defaults ─────────────────────────────────────────────────────────
constexpr float DEFAULT_DELTA_TIME       = 1.f / 60.f;
constexpr float DEFAULT_SPRING_STIFFNESS = 500.f;
constexpr float DEFAULT_BEND_STIFFNESS   = 50.f;
constexpr float DEFAULT_SPRING_DAMPING   = 0.1f;
constexpr float DEFAULT_AIR_DAMPING      = 0.01f;
constexpr float DEFAULT_PARTICLE_MASS    = 1.f;
constexpr float DEFAULT_MAX_STRETCH      = 1.10f;
constexpr float DEFAULT_MAX_COMPRESS     = 0.90f;
constexpr int   DEFAULT_CONSTRAINT_ITERS = 15;
constexpr glm::vec3 DEFAULT_GRAVITY      = {0.0f, -9.8f, 0.0f};

// ── Rendering ────────────────────────────────────────────────────────────────
constexpr float DEFAULT_POINT_SIZE  = 4.f;

constexpr float CAMERA_FOV  = 45.f;
constexpr float CAMERA_NEAR = 0.01f;
constexpr float CAMERA_FAR  = 100.f;

inline const glm::vec3 DEFAULT_CAMERA_POS    = { 0.f,  1.0f, 10.0f };
inline const glm::vec3 DEFAULT_CAMERA_TARGET = { 0.f,  1.0f, 0.f };
inline const glm::vec3 DEFAULT_CAMERA_UP     = { 0.f,  1.0f, 0.f };
inline const glm::vec3 DEFAULT_LIGHT_POS     = { 2.f,  4.f,  3.f };
inline const glm::vec3 DEFAULT_CLOTH_COLOR   = { 0.7f, 0.5f, 0.9f };
