#version 330 core
in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec4 FragColor;

void main()
{
    vec3 norm     = normalize(gl_FrontFacing ? vNormal : -vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 viewDir  = normalize(uViewPos  - vFragPos);
    vec3 halfDir  = normalize(lightDir + viewDir);

    vec3 ambient  = 0.15 * uColor;
    vec3 diffuse  = max(dot(norm, lightDir), 0.0) * uColor;
    float spec    = pow(max(dot(norm, halfDir), 0.0), 256.0);
    vec3 specular = 0.4 * spec * vec3(1.0);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
