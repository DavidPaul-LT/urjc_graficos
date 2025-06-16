#version 330 core

out vec4 outColor;

in vec3 color;
in vec3 pos; // Coordenadas de cámara
in vec3 pos2; // Coordenadas del mundo virtual
in vec3 norm;
in vec2 texCoord;
flat in int vCubeId;

uniform sampler2D colorTex;
uniform sampler2D emiTex;
uniform vec3 cameraPos;
uniform float uTime; // Tiempo para cambiar la intensidad de la luz

//Propiedades del objeto
vec3 Ka;
vec3 Kd;
vec3 Ks;
vec3 N;
float alpha = 1700.0;
vec3 Ke;

// Luz focal
vec3 spotDir = normalize(vec3(0.0, 0.0, -1.0));
vec3 spotId = vec3(2.0); // Intensidad de la luz difusa
vec3 spotIs = vec3(2.0); // Intensidad de la luz especular 
vec3 spotPos = vec3(0.0, 0.0, 0.0); // Posición de la luz focal

//Propiedades de la luz
vec3 Ia = vec3 (0.3);
vec3 Id = vec3 (1.0);
vec3 Is = vec3 (1.0);

//Densidad y color de la niebla
float density = 0.05;
vec3 bg = vec3(0.2,0.2,0.2);

// Luz dinámica
vec3 lposdynamic = vec3(2.0 * sin(uTime)+4.0, sin(uTime)/2.0, 2.0 * cos(uTime)+5.0); // Posición móvil de la luz
vec3 lcolordynamic = vec3(1.0,0.0,0.0);

vec3 shade();

void main()
{   
	Ka = texture(colorTex, texCoord).rgb;
    Kd = texture(colorTex, texCoord).rgb;
    Ke = texture(emiTex, texCoord).rgb;
    Ks = vec3(1.0);

    N = normalize(norm);

    outColor = vec4(shade(), 1.0);
   
}

vec3 shade()
{
	vec3 c = vec3(0.0);
	c = Ia * Ka;

    // Luz focal (sigue a la cámara, usa pos)
    vec3 fragmentToLight = normalize(spotPos - pos);

    // Función de ventana para atenuación con distancia para la luz focal
    float cutoff = 10.0;
    float penumbra = 5.0;
    float distanceToFocalLight = length(spotPos - pos);  // Distancia de la luz focal
    float maxDistance = 15.0;
    float d0 = 1.0;
    float epsilon = sqrt(0.001);
    float f_win = pow(max(1.0 - pow(distanceToFocalLight / maxDistance, 4.0), 0.0), 2.0);
    float t = clamp((dot(-fragmentToLight,spotDir)-cos(radians(cutoff)))/(cos(radians(penumbra))-cos(radians(cutoff))),0,1);
    float attenuation = pow(d0 / (distanceToFocalLight + epsilon), 2.0) * f_win * smoothstep(0,1,t);

    // Componente difusa
    float diff = max(dot(fragmentToLight, N), 0.0);
    vec3 diffuse = spotId * Kd * diff;

    vec3 V = normalize(cameraPos-pos2);
    vec3 H = normalize(fragmentToLight + V);
    float specFactor = max(dot(N, H), 0.0);
    vec3 specular = spotIs * Ks * pow(specFactor, alpha);

    c += attenuation * (diffuse + specular);

	// Luz dinámica que varía su intensidad
    vec3 dynamicL = normalize(lposdynamic - pos2); // Vector hacia la luz
    float dynamicDistance = length(lposdynamic - pos2);
    float dynamicIntensity = abs(sin(uTime *2.0)); // Intensidad oscilante con el tiempo

    float powy = pow(dynamicDistance / maxDistance, 4.0);
    float maxy = max(1.0 - powy, 0.0);
    float f_win_dynamic = pow(maxy, 2.0);
    float dynamic_attenuation = (pow(d0, 2.0) / (dynamicDistance + epsilon)) * f_win_dynamic ;

    float dynamicDiffuseFactor = max(dot(dynamicL, N), 0.0);
    vec3 dynamicDiffuse = lcolordynamic * dynamicIntensity * Kd * dynamicDiffuseFactor;

    vec3 dynamicV = normalize(cameraPos-pos2);
    vec3 dynamicH = normalize(dynamicL + dynamicV);
    float dynamicSpecFactor = max(dot(N, dynamicH), 0.0);
    vec3 dynamicSpecular = lcolordynamic * dynamicIntensity * Ks * pow(dynamicSpecFactor, alpha);

    c += (dynamicDiffuse + dynamicSpecular)* dynamic_attenuation;

    // Emisión
    c += Ke;

    // Niebla
    float p = 1.0/exp(pow(density* length(vec3(0)-pos),2));
	   
	return clamp(c*p + (1-p)*bg, 0.0, 1.0);
}
