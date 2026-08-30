namespace WildStar.Sky;

public static class SkyShaders
{
    public const string SkyDome = @"shader_type sky;
uniform vec3 sky_sh[9];
uniform vec4 sky_bands[16];
uniform float dome_weight = 1.0;
uniform float dome_yaw = 0.0;
uniform float dome_pitch = 0.0;

const float RING_Y[14] = float[14](7.5957541, 3.7320508, 2.4142136, 1.7320508, 1.3032254, 1.0, 0.7673270, 0.5773503, 0.4142136, 0.2679492, 0.1316525, 0.0, -0.5773503, -1.7320508);
const float APEX_Y = 10.0;
const float APEX_DROP = 2.4042459;
const float NADIR_Y = -5.0;
const float NADIR_RISE = 3.2679492;

vec3 evaluate_sh(vec3 n) {
    float x = n.x;
    float y = n.y;
    float z = n.z;
    vec3 c = sky_sh[0] * 0.28209481;
    c += sky_sh[1] * (-0.48860252 * y);
    c += sky_sh[2] * (0.48860252 * z);
    c += sky_sh[3] * (-0.48860252 * x);
    c += sky_sh[4] * (1.0925485 * x * y);
    c += sky_sh[5] * (-1.0925485 * z * y);
    c += sky_sh[6] * (0.31539157 * (3.0 * z * z - 1.0));
    c += sky_sh[7] * (-1.0925485 * x * z);
    c += sky_sh[8] * (0.54627424 * (x * x - y * y));
    return c;
}

vec4 band_colour(vec3 dir) {
    float sin_e = clamp(dir.y, -1.0, 1.0);
    float cos_e = sqrt(max(1.0 - sin_e * sin_e, 0.0));
    if (sin_e >= 0.99144486) {
        float t = APEX_Y / (APEX_DROP * cos_e + sin_e);
        float apex = clamp(1.0 - t * cos_e, 0.0, 1.0);
        return mix(sky_bands[1], sky_bands[0], apex);
    }
    if (sin_e <= -0.86602540) {
        float t = -NADIR_Y / (NADIR_RISE * cos_e - sin_e);
        float nadir = clamp(1.0 - t * cos_e, 0.0, 1.0);
        return mix(sky_bands[14], sky_bands[15], nadir);
    }
    float y = sin_e / max(cos_e, 0.0001);
    for (int k = 0; k < 13; k++) {
        if (y >= RING_Y[k + 1]) {
            float t = (y - RING_Y[k + 1]) / (RING_Y[k] - RING_Y[k + 1]);
            return mix(sky_bands[k + 2], sky_bands[k + 1], t);
        }
    }
    return sky_bands[14];
}

vec3 rotate_dome(vec3 d) {
    float yaw = radians(dome_yaw);
    float pitch = radians(dome_pitch);
    float cy = cos(yaw);
    float sy = sin(yaw);
    float cp = cos(pitch);
    float sp = sin(pitch);
    vec3 r = vec3(cy * d.x - sy * d.z, d.y, sy * d.x + cy * d.z);
    return vec3(r.x, cp * r.y - sp * r.z, sp * r.y + cp * r.z);
}

vec3 srgb_to_linear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}

void sky() {
    vec3 dir = rotate_dome(normalize(EYEDIR));
    vec3 c = evaluate_sh(dir) + band_colour(dir).rgb * step(0.0001, dome_weight);
    c = min(max(c, vec3(0.0)), vec3(1.5));
    COLOR = srgb_to_linear(c);
}
";

    public const string FogCompute = @"#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, set = 0, binding = 0) uniform restrict image2D color_image;
layout(set = 0, binding = 1) uniform sampler2D depth_tex;
layout(set = 0, binding = 2, std430) restrict readonly buffer Params {
    mat4 inv_proj;
    mat4 cam_basis;
    vec4 size_mid_den;
    vec4 flags;
    vec4 near_far;
    vec4 fog_sh[9];
    vec4 fog_bands[16];
} p;

const float RING_Y[14] = float[14](7.5957541, 3.7320508, 2.4142136, 1.7320508, 1.3032254, 1.0, 0.7673270, 0.5773503, 0.4142136, 0.2679492, 0.1316525, 0.0, -0.5773503, -1.7320508);

vec3 evaluate_sh(vec3 n) {
    float x = n.x; float y = n.y; float z = n.z;
    vec3 c = p.fog_sh[0].rgb * 0.28209481;
    c += p.fog_sh[1].rgb * (-0.48860252 * y);
    c += p.fog_sh[2].rgb * (0.48860252 * z);
    c += p.fog_sh[3].rgb * (-0.48860252 * x);
    c += p.fog_sh[4].rgb * (1.0925485 * x * y);
    c += p.fog_sh[5].rgb * (-1.0925485 * z * y);
    c += p.fog_sh[6].rgb * (0.31539157 * (3.0 * z * z - 1.0));
    c += p.fog_sh[7].rgb * (-1.0925485 * x * z);
    c += p.fog_sh[8].rgb * (0.54627424 * (x * x - y * y));
    return c;
}

vec4 band_colour(vec3 dir) {
    float sin_e = clamp(dir.y, -1.0, 1.0);
    float cos_e = sqrt(max(1.0 - sin_e * sin_e, 0.0));
    if (sin_e >= 0.99144486) {
        float t = 10.0 / (2.4042459 * cos_e + sin_e);
        return mix(p.fog_bands[1], p.fog_bands[0], clamp(1.0 - t * cos_e, 0.0, 1.0));
    }
    if (sin_e <= -0.86602540) {
        float t = 5.0 / (3.2679492 * cos_e - sin_e);
        return mix(p.fog_bands[14], p.fog_bands[15], clamp(1.0 - t * cos_e, 0.0, 1.0));
    }
    float y = sin_e / max(cos_e, 0.0001);
    for (int k = 0; k < 13; k++) {
        if (y >= RING_Y[k + 1]) {
            float t = (y - RING_Y[k + 1]) / (RING_Y[k] - RING_Y[k + 1]);
            return mix(p.fog_bands[k + 2], p.fog_bands[k + 1], t);
        }
    }
    return p.fog_bands[14];
}

vec3 srgb_to_linear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 linear_to_srgb(vec3 c) {
    c = max(c, vec3(0.0));
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

void main() {
    ivec2 size = ivec2(p.size_mid_den.xy);
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    if (pix.x >= size.x || pix.y >= size.y) {
        return;
    }
    float depth = texelFetch(depth_tex, pix, 0).r;
    if (depth <= 0.0 || depth >= 1.0) {
        return;
    }
    vec2 uv = (vec2(pix) + 0.5) / vec2(size);
    vec2 ndc = uv * 2.0 - 1.0;
    if (p.flags.y > 0.5) {
        ndc.y = -ndc.y;
    }
    // The depth buffer is reversed and remapped to [0, 1] for the RD backends while the
    // projection handed to effects is the GL-style one; try that convention first and fall
    // back to a plain [-1, 1] depth if the result is outside the camera's range.
    vec4 view = p.inv_proj * vec4(ndc, 1.0 - 2.0 * depth, 1.0);
    view.xyz /= view.w;
    float near_z = p.near_far.x;
    float far_z = p.near_far.y;
    if (!(view.z < 0.0 && -view.z >= near_z * 0.99 && -view.z <= far_z * 1.01)) {
        vec4 alt = p.inv_proj * vec4(ndc, depth * 2.0 - 1.0, 1.0);
        alt.xyz /= alt.w;
        if (alt.z < 0.0 && -alt.z >= near_z * 0.99 && -alt.z <= far_z * 1.01) {
            view = alt;
        }
    }
    float d = length(view.xyz);
    vec3 dir = normalize((p.cam_basis * vec4(view.xyz, 0.0)).xyz);
    vec4 band = band_colour(dir);
    float a = band.a * step(0.0001, p.flags.z);
    float mid2 = p.size_mid_den.z;
    float inv_den = p.size_mid_den.w;
    float fog = clamp((1.0 - exp2(-a * (d * d + d) / mid2)) * inv_den, 0.0, 1.0);
    if (p.flags.x > 0.5) {
        imageStore(color_image, pix, vec4(abs(dir), 1.0));
        return;
    }
    vec3 fog_colour = min(max(2.0 * evaluate_sh(dir) * band.rgb, vec3(0.0)), vec3(1.5));
    vec4 scene = imageLoad(color_image, pix);
    vec3 blended = mix(linear_to_srgb(scene.rgb), fog_colour, fog);
    imageStore(color_image, pix, vec4(srgb_to_linear(blended), scene.a));
}
";

    public const string GradeCompute = @"#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, set = 0, binding = 0) uniform restrict image2D color_image;
layout(set = 0, binding = 1) uniform sampler3D lut_tex;
layout(set = 0, binding = 2, std430) restrict readonly buffer Params {
    vec4 size_flags;
    vec4 sepia;
    vec4 sat_con_bri_gam;
    vec4 lut_gamma;
} p;

vec3 srgb_to_linear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 linear_to_srgb(vec3 c) {
    c = max(c, vec3(0.0));
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

void main() {
    ivec2 size = ivec2(p.size_flags.xy);
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    if (pix.x >= size.x || pix.y >= size.y) {
        return;
    }
    vec4 scene = imageLoad(color_image, pix);
    vec3 c = linear_to_srgb(scene.rgb);
    c = pow(max(c, vec3(0.0)), vec3(p.sat_con_bri_gam.w));
    float lum = dot(c, vec3(0.2125, 0.7154, 0.0721));
    vec3 tint = p.sepia.rgb * lum;
    c = (c - tint) * p.sat_con_bri_gam.x + tint;
    vec3 pivot = p.sepia.rgb * p.sepia.a;
    c = (c - pivot) * p.sat_con_bri_gam.y + pivot;
    c = c * p.sat_con_bri_gam.z;
    if (p.lut_gamma.x > 0.5) {
        c = texture(lut_tex, clamp(c, 0.0, 1.0) * 0.9375 + 0.03125).rgb;
    }
    c = pow(max(c, vec3(0.0)), vec3(p.lut_gamma.y));
    imageStore(color_image, pix, vec4(srgb_to_linear(c), scene.a));
}
";
}
