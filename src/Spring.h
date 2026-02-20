#pragma once

enum class SpringType
{
    Structural, // immediate neighbors (left/right/up/down)
    Shear,      // diagonal neighbors (prevents face collapse)
    Bending     // two-ring neighbors (resists bending/wrinkling)
};

struct Spring
{
    int        a, b;        // indices into the particle array
    float      restLength;
    float      stiffness;   // k  (Hooke's Law)
    float      damping;     // damping coefficient
    SpringType type;
};
