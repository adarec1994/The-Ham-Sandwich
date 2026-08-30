namespace WildStar.Model;

public static class M3ParticleShaders
{
    public static string Sprite(int blendMode, bool depthTest, bool depthWrite)
    {
        string blend = blendMode switch
        {
            3 or 9 or 10 => "blend_add",
            4 => "blend_add",
            5 or 6 => "blend_mul",
            8 => "blend_sub",
            _ => "blend_mix",
        };

        string depth = depthWrite ? "depth_draw_always" : "depth_draw_never";
        string test = depthTest ? string.Empty : ", depth_test_disabled";
        bool cutout = blendMode == 1;
        bool additiveOne = blendMode is 3 or 9 or 10;

        return "shader_type spatial;\n" +
               "render_mode unshaded, cull_disabled, skip_vertex_transform, " + blend + ", " + depth + test + ";\n" +
               "uniform sampler2D albedo : source_color, filter_linear_mipmap, repeat_enable;\n" +
               "uniform vec4 tint : source_color = vec4(1.0);\n" +
               "uniform vec2 uv_scale = vec2(1.0);\n" +
               "uniform float columns = 1.0;\n" +
               "uniform float luminance_mode = 0.0;\n" +
               "varying vec4 particle_colour;\n" +
               "void vertex() {\n" +
               "    vec3 centre = (MODELVIEW_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;\n" +
               "    float size = INSTANCE_CUSTOM.x;\n" +
               "    float angle = INSTANCE_CUSTOM.y;\n" +
               "    float c = cos(angle);\n" +
               "    float s = sin(angle);\n" +
               "    vec2 corner = VERTEX.xy;\n" +
               "    vec2 offset = vec2(c * corner.x - s * corner.y, s * corner.x + c * corner.y) * size;\n" +
               "    VERTEX = centre + vec3(offset, 0.0);\n" +
               "    NORMAL = vec3(0.0, 0.0, 1.0);\n" +
               "    float sign_u = mod(INSTANCE_CUSTOM.w, 2.0) >= 1.0 ? 1.0 : -1.0;\n" +
               "    float sign_v = INSTANCE_CUSTOM.w >= 2.0 ? 1.0 : -1.0;\n" +
               "    vec2 uv = (UV - 0.5) * vec2(sign_u, sign_v) + 0.5;\n" +
               "    float frame = INSTANCE_CUSTOM.z;\n" +
               "    vec2 cell = vec2(mod(frame, columns), floor(frame / columns));\n" +
               "    UV = (uv + cell) * uv_scale;\n" +
               "    particle_colour = COLOR;\n" +
               "}\n" +
               "void fragment() {\n" +
               "    vec4 tex = texture(albedo, UV);\n" +
               "    vec3 colour = tex.rgb * particle_colour.rgb;\n" +
               "    if (luminance_mode > 0.5) {\n" +
               "        colour = vec3(dot(vec3(0.21, 0.72, 0.07), colour));\n" +
               "    }\n" +
               "    float alpha = tex.a * particle_colour.a;\n" +
               (additiveOne
                   ? "    ALBEDO = colour * tint.rgb;\n    ALPHA = 1.0;\n"
                   : "    ALBEDO = colour * tint.rgb;\n    ALPHA = alpha;\n") +
               (cutout ? "    ALPHA_SCISSOR_THRESHOLD = 0.5;\n" : string.Empty) +
               "}\n";
    }
}
