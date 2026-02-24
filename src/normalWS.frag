#version 330 core
in vec3 vNormalWS;

out vec4 FragColor;

void main()
{
    // Normalize the interpolated normal
    vec3 norm = normalize(vNormalWS);

    // Use absolute values for more dramatic visualization:
    // Shows direction clearly regardless of orientation
    // X: red, Y: green, Z: blue
    vec3 absNorm = abs(norm);

    FragColor = vec4(absNorm, 1.0);
}

