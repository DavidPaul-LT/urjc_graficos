#version 330 core

out vec4 outColor;

in vec3 color;
in vec3 posCam; // Coordenadas de cámara
in vec3 posWorld; // Coordenadas del mundo virtual
in vec3 normCam;  // Normal de la cámara
in vec3 normWorld; // Normal del mundo
in vec2 texCoord;
flat in int vCubeId;

uniform sampler2D colorTex;
uniform sampler2D emiTex;
uniform vec3 cameraPos;

uniform float uTime;

// propiedades de la luz direccional

uniform vec3 dirLightDir;  // Dirección
uniform vec3 dirLightId;   // Intensidad difusa
uniform vec3 dirLightIs;   // Intensidad especular


// Propiedades del objeto
vec3 Ka;
vec3 Kd;
vec3 Ks;
vec3 N;
float alpha = 100.0; // A mayor valor, más "rugoso" es el material para reflejar luz
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
float density = 0.08;
vec3 bg = vec3(0.05, 0.1, 0.05);

// función de círculo blanco en fondo negro
vec3 proceduralCircle(vec2 uv) {
    float d = distance(uv, vec2(0.5));
    if (d < 0.2)
        return vec3(1.0);  // blanco dentro del círculo
    else
        return vec3(0.0);  // negro fuera
}

// función de bandas coloreadas animadas
vec3 movingStripes(vec2 uv, float time) {
    float bands = sin((uv.x + time) * 10.0) * 0.5 + 0.5;
    return vec3(bands, bands * 0.5, 1.0 - bands);
}

vec3 shade();

void main()
{
    // Color por función matemática

    if (vCubeId == 2) {
        Ka = movingStripes(texCoord, uTime);
        Kd = Ka;
    } else {
        Ka = proceduralCircle(texCoord);
        Kd = Ka;
    }
        
    Ke = texture(emiTex, texCoord).rgb;
    Ks = vec3(1.0);

    N = normalize(normCam);

    // --- NIEBLA EXPONENCIAL CUADRÁTICA ---

    float fogDensity = 0.045;
    float fogDistance = length(cameraPos - posCam);
    float fogFactor = exp(-pow(fogDensity * fogDistance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 fogColor = vec3(0.05, 0.1, 0.05); // Niebla suave de color verde
    vec3 finalColor = mix(fogColor, shade(), fogFactor);
    outColor = vec4(finalColor, 1.0);
}

vec3 shade()
{
    vec3 c = Ia * Ka;
    N = normalize(normWorld);

    // Iteración sobre las luces puntuales
    for (int i = 0; i < 2; ++i)
    {
        vec3 fragPos = posWorld; 
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
    N = normalize(normCam); // ← normal en coords de cámara
    vec3 fragmentToLight = normalize(spotPos - posCam);

    float cutoff = 30.0;
    float penumbra = 20.0;
    float distanceToFocalLight = length(spotPos - posCam);
    float maxDistance = 15.0;
    float d0 = 1.0;
    float epsilon = sqrt(0.001);
    float f_win = pow(max(1.0 - pow(distanceToFocalLight / maxDistance, 4.0), 0.0), 2.0);
    float t = clamp((dot(-fragmentToLight,spotDir)-cos(radians(cutoff)))/(cos(radians(penumbra))-cos(radians(cutoff))),0,1);
    float attenuation = pow(d0 / (distanceToFocalLight + epsilon), 2.0) * f_win * t;
    attenuation = min(attenuation, 1.0); // evita saturación
    attenuation *= smoothstep(0.3, 1.0, distanceToFocalLight);

    float diff = max(dot(fragmentToLight, N), 0.0);
    vec3 diffuse = spotId * Kd * diff;

    vec3 V = normalize(cameraPos - posWorld); 
    vec3 H = normalize(fragmentToLight + V);
    float specFactor = max(dot(N, H), 0.0);
    vec3 specular = spotIs * Ks * pow(specFactor, alpha);

    c += attenuation * (diffuse + specular);

    // Luz direccional
    vec3 Ld = normalize(-dirLightDir);
    float diffD = max(dot(N, Ld), 0.0);

    vec3 Vw = normalize(cameraPos - posWorld);
    vec3 Hd = normalize(Ld + Vw);
    float specD = pow(max(dot(N, Hd), 0.0), alpha);

    vec3 diffuseDir = dirLightId * Kd * diffD;
    vec3 specularDir = dirLightIs * Ks * specD;
    c += diffuseDir + specularDir;

    // Emisión
    c += Ke;

    // Niebla
    float p = 1.0 / exp(pow(density * length(vec3(0) - posCam), 2));
    return clamp(c * p + (1 - p) * bg, 0.0, 1.0);
}
