#ifndef COMMONS_SKY_
#define COMMONS_SKY_

#include <constants>

vec3 pick_sky_color(samplerCube cubemap, float dayTime) {
    float elevation = sin(dayTime * PI2) * 0.2;
    vec3 skyLightColor = texture(cubemap, vec3(0.0f, elevation, -0.4f)).rgb;
    skyLightColor *= SKY_LIGHT_TINT;
    skyLightColor = min(vec3(1.0f), skyLightColor * SKY_LIGHT_MUL);
    skyLightColor = max(MIN_SKY_LIGHT, skyLightColor);
    return skyLightColor;
}

#endif // COMMONS_SKY_
