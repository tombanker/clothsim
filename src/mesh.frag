#version 330 core
in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec4 FragColor;

float Pow5(float n){
    return n * n * n * n * n;
}

vec3 PhongLighting(vec3 normal, vec3 lightDir, vec3 halfDir){
    vec3 ambient  = 0.15 * uColor;
    vec3 diffuse  = max(dot(normal, lightDir), 0.0) * uColor;
    float spec    = pow(max(dot(normal, halfDir), 0.0), 256.0);
    vec3 specular = 0.4 * spec * vec3(1.0);
    return ambient + diffuse + specular;
}

vec3 FresnelPhongLighting(vec3 normal, vec3 lightDir, vec3 halfDir, vec3 viewDir){
    vec3 ambient  = 0.15 * uColor;
    vec3 diffuse  = max(dot(normal, lightDir), 0.0) * uColor;

    float NdotV = dot(normal, viewDir);
    vec3 fresnel = vec3(Pow5(1.0 - NdotV));
    fresnel = mix(vec3(0.04), uColor, fresnel);
    vec3 specular = fresnel * pow(max(dot(normal, halfDir), 0.0), 32.0);

    return ambient + diffuse + specular;
}

void main()
{
    vec3 N = normalize(gl_FrontFacing ? vNormal : -vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    vec3 V = normalize(uViewPos  - vFragPos);
    vec3 H  = normalize(L + V);

    vec3 col;
    vec3 phong = FresnelPhongLighting(N, L, H, V);
    col = phong;

    FragColor = vec4(col, 1.0);
}
