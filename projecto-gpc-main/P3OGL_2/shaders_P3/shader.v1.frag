#version 330 core

out vec4 outColor;

in vec3 color;
in vec3 pos; // Coordenadas de cámara
in vec3 pos2; // Coordenadas del mundo virtual
in vec3 norm_world;
in vec3 norm;
in vec2 texCoord;
flat in int vCubeId;

uniform sampler2D colorTex;
uniform sampler2D emiTex;
uniform vec3 cameraPos;

uniform vec3 dirLightDir;  // Dirección de la luz direccional
uniform vec3 dirLightId;   // Intensidad difusa de la luz direccional
uniform vec3 dirLightIs;   // Intensidad especular de la luz direccional


// Propiedades del objeto
vec3 Ka;
vec3 Kd;
vec3 Ks;
vec3 N;
float alpha = 5000.0; // Afecta a la rugosidad - Controla el brillo de la reflexión especular
vec3 Ke;

// Luz focal
vec3 spotDir = normalize(vec3(0.0, 0.0, -1.0));
vec3 spotId = vec3(2.0); // Intensidad de la luz difusa
vec3 spotIs = vec3(2.0); // Intensidad de la luz especular 
vec3 spotPos = vec3(0.0, 0.0, 0.0); // Posición de la luz focal

// Propiedades de las luces puntuales (2 fuentes de luz)
vec3 Ia = vec3(0.3);
vec3 Id[2] = vec3[2](vec3(0.8), vec3(0.5));  // Dos fuentes de luz difusa
vec3 Is[2] = vec3[2](vec3(0.8), vec3(0.5));  // Dos fuentes de luz especular
vec3 lpos[2] = vec3[2](vec3(2.0, 1.0, 2.0), vec3(-2.0, 1.0, 1.0));   // Posiciones de luces puntuales


//Densidad y color de la niebla
float density = 0.05;
vec3 bg = vec3(0.2,0.2,0.2);

vec3 generatePattern(vec2 uv) {
    // Patrón más llamativo para el cubo central
    float stripes = sin(uv.x * 20.0) * sin(uv.y * 20.0); // Aumenta la frecuencia del patrón (más detalles)
    float r = sin(uv.x * 5.0 + uv.y * 3.0) * 0.5 + 0.5; // Franjas diagonales rojas
    float g = cos(uv.y * 7.0 - uv.x * 2.0) * 0.5 + 0.5;  // Ondas verdes
    float b = (sin(uv.x * 3.0) * cos(uv.y * 5.0)) * 0.5 + 0.5; // Patrón modular azul
    return mix(vec3(r, g, b), vec3(stripes), 0.3); //  * 0.5 + 0.5: Transforma el rango de salida de salida -1 en algo que se pueda colorear
}

vec3 shade();

void main()
{
    // Cubo 2 usa patrón matemático, otros usan textura
    if (vCubeId == 2) {
        Ka = generatePattern(texCoord);
        Kd = generatePattern(texCoord);
    } else {
        Ka = texture(colorTex, texCoord).rgb;
        Kd = texture(colorTex, texCoord).rgb;
    }
    
    Ke = texture(emiTex, texCoord).rgb;
    Ks = vec3(1.0);

    N = normalize(norm);

    outColor = vec4(shade(), 1.0);
}

vec3 shade()
{
    vec3 c = Ia * Ka;
    N = normalize(norm_world); // Reutilizas N para todo el shading

    // Iteramos solo sobre las luces puntuales
    for (int i = 0; i < 2; ++i)
    {
        vec3 fragPos = pos2; 
        vec3 L = normalize(lpos[i] - fragPos);
        float distance = length(lpos[i] - fragPos);

        float diffFactor = max(dot(L, N), 0.0);
        vec3 diffuse = Id[i] * Kd * diffFactor;

        vec3 V = normalize(cameraPos - fragPos);
        vec3 H = normalize(L + V);
        float specFactor = max(dot(N, H), 0.0);
        vec3 specular = Is[i] * Ks * pow(specFactor, alpha);

        c += diffuse + specular;
    }

    // Luz focal
    N = normalize(norm); // ← normal en coords de cámara
    vec3 fragmentToLight = normalize(spotPos - pos);

    float cutoff = 30.0;
    float penumbra = 20.0;
    float distanceToFocalLight = length(spotPos - pos);
    float maxDistance = 15.0;
    float d0 = 1.0;
    float epsilon = sqrt(0.001);
    float f_win = pow(max(1.0 - pow(distanceToFocalLight / maxDistance, 4.0), 0.0), 2.0);
    float t = clamp((dot(-fragmentToLight,spotDir)-cos(radians(cutoff)))/(cos(radians(penumbra))-cos(radians(cutoff))),0,1);
    float attenuation = pow(d0 / (distanceToFocalLight + epsilon), 2.0) * f_win * t;

    float diff = max(dot(fragmentToLight, N), 0.0);
    vec3 diffuse = spotId * Kd * diff;

    vec3 V = normalize(cameraPos - pos2); 
    vec3 H = normalize(fragmentToLight + V);
    float specFactor = max(dot(N, H), 0.0);
    vec3 specular = spotIs * Ks * pow(specFactor, alpha);

    c += attenuation * (diffuse + specular);

    // Luz direccional
    vec3 Ld = normalize(-dirLightDir);
    float diffD = max(dot(N, Ld), 0.0);

    vec3 Vw = normalize(cameraPos - pos2);
    vec3 Hd = normalize(Ld + Vw);
    float specD = pow(max(dot(N, Hd), 0.0), alpha);

    vec3 diffuseDir = dirLightId * Kd * diffD;
    vec3 specularDir = dirLightIs * Ks * specD;
    c += diffuseDir + specularDir;

    // Emisión
    c += Ke;

    // Niebla
    float p = 1.0 / exp(pow(density * length(vec3(0) - pos), 2));
    return clamp(c * p + (1 - p) * bg, 0.0, 1.0);
}
