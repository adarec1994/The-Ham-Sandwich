bl_info = {
    "name": "WildStar Terrain Importer",
    "author": "Matthew Wakeman",
    "version": (1, , 0),
    "blender": (3, 0, 0),
    "location": "",
    "description": "",
    "category": "",
}

import bpy
import json
import base64
import os
import tempfile
from bpy.props import StringProperty
from bpy_extras.io_utils import ImportHelper


class IMPORT_OT_wsterrain(bpy.types.Operator, ImportHelper):
    bl_idname = "import_scene.wsterrain"
    bl_label = "Import WildStar Terrain"
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = ".wsterrain"
    filter_glob: StringProperty(default="*.wsterrain", options={'HIDDEN'})

    def execute(self, context):
        return import_terrain(context, self.filepath)


def menu_func_import(self, context):
    self.layout.operator(IMPORT_OT_wsterrain.bl_idname, text="WildStar Terrain (.wsterrain)")


def register():
    bpy.utils.register_class(IMPORT_OT_wsterrain)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.utils.unregister_class(IMPORT_OT_wsterrain)


def create_solid_color_image(name, r, g, b, a=255):
    img = bpy.data.images.new(name, width=4, height=4, alpha=True)
    pixels = []
    for _ in range(16):
        pixels.extend([r/255.0, g/255.0, b/255.0, a/255.0])
    img.pixels[:] = pixels
    img.pack()
    img.colorspace_settings.name = 'Non-Color'
    return img


def load_image_from_base64(name, b64_data, flip_v=False):
    png_data = base64.b64decode(b64_data)
    with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as f:
        f.write(png_data)
        temp_path = f.name
    img = bpy.data.images.load(temp_path)
    img.name = name

    if flip_v and img.size[0] > 0 and img.size[1] > 0:
        width, height = img.size
        pixels = list(img.pixels[:])
        flipped = []
        for y in range(height - 1, -1, -1):
            row_start = y * width * 4
            row_end = row_start + width * 4
            flipped.extend(pixels[row_start:row_end])
        img.pixels[:] = flipped
        img.update()

    img.pack()
    img.colorspace_settings.name = 'Non-Color'
    os.unlink(temp_path)
    return img


def create_splatmap_material(chunk_name, chunk_data, layers_data, layer_images, blend_images, color_images):
    mat = bpy.data.materials.new(name=f"{chunk_name}_Mat")
    mat.use_nodes = True
    mat.blend_method = 'OPAQUE'
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (3200, 0)

    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (2900, 0)
    bsdf.inputs['Roughness'].default_value = 0.85
    if 'Specular IOR Level' in bsdf.inputs:
        bsdf.inputs['Specular IOR Level'].default_value = 0.1
    elif 'Specular' in bsdf.inputs:
        bsdf.inputs['Specular'].default_value = 0.1
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    uv_blend = nodes.new('ShaderNodeUVMap')
    uv_blend.location = (-1800, -400)
    uv_blend.uv_map = "UVMap"

    uv_tex = nodes.new('ShaderNodeUVMap')
    uv_tex.location = (-1800, 200)
    uv_tex.uv_map = "UVMap"

    layer_ids = chunk_data.get('layerIds', [0, 0, 0, 0])
    chunk_idx = chunk_data.get('index', 0)
    blend_img = blend_images.get(f"chunk_{chunk_idx}_blend")

    valid_slots = []
    for i, lid in enumerate(layer_ids):
        if lid != 0 and f"layer_{lid}_diffuse" in layer_images:
            valid_slots.append(i)

    if not valid_slots:
        rgb = nodes.new('ShaderNodeRGB')
        rgb.location = (1200, 0)
        rgb.outputs[0].default_value = (1.0, 0.0, 1.0, 1.0)
        links.new(rgb.outputs[0], bsdf.inputs['Base Color'])
        return mat

    def get_layer_scale(layer_id):
        layer_info = layers_data.get(str(layer_id), {})
        meters_per_tile = layer_info.get('scale', 4.0)
        if meters_per_tile <= 0:
            meters_per_tile = 4.0
        return 32.0 / meters_per_tile

    mapping_nodes = {}

    def get_or_create_mapping(layer_id, y_offset):
        if layer_id in mapping_nodes:
            return mapping_nodes[layer_id]
        scale = get_layer_scale(layer_id)
        mapping = nodes.new('ShaderNodeMapping')
        mapping.location = (-1400, y_offset)
        mapping.inputs['Scale'].default_value = (scale, scale, 1.0)
        links.new(uv_tex.outputs['UV'], mapping.inputs['Vector'])
        mapping_nodes[layer_id] = mapping
        return mapping

    def create_layer_diffuse(slot_idx, layer_id, y_offset):
        diffuse_img = layer_images.get(f"layer_{layer_id}_diffuse")
        if not diffuse_img:
            return None
        mapping = get_or_create_mapping(layer_id, y_offset)
        tex = nodes.new('ShaderNodeTexImage')
        tex.location = (-1000, y_offset)
        tex.image = diffuse_img
        tex.interpolation = 'Linear'
        links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
        return tex.outputs['Color']

    def create_layer_normal(slot_idx, layer_id, y_offset):
        normal_img = layer_images.get(f"layer_{layer_id}_normal")
        if not normal_img:
            return None
        mapping = get_or_create_mapping(layer_id, y_offset)
        tex = nodes.new('ShaderNodeTexImage')
        tex.location = (-600, y_offset)
        tex.image = normal_img
        tex.interpolation = 'Linear'
        links.new(mapping.outputs['Vector'], tex.inputs['Vector'])
        return tex.outputs['Color']

    def apply_color_map(final_color):
        color_img = color_images.get(f"chunk_{chunk_idx}_color")
        if color_img:
            color_tex = nodes.new('ShaderNodeTexImage')
            color_tex.location = (1900, -300)
            color_tex.image = color_img
            color_tex.interpolation = 'Linear'
            color_tex.extension = 'EXTEND'
            links.new(uv_blend.outputs['UV'], color_tex.inputs['Vector'])

            mult_2 = nodes.new('ShaderNodeVectorMath')
            mult_2.operation = 'SCALE'
            mult_2.location = (2100, -300)
            mult_2.inputs['Scale'].default_value = 2.0
            links.new(color_tex.outputs['Color'], mult_2.inputs[0])

            mult_color = nodes.new('ShaderNodeVectorMath')
            mult_color.operation = 'MULTIPLY'
            mult_color.location = (2300, 0)
            links.new(final_color, mult_color.inputs[0])
            links.new(mult_2.outputs[0], mult_color.inputs[1])
            return mult_color.outputs[0]
        return final_color

    if len(valid_slots) == 1 or blend_img is None:
        slot = valid_slots[-1]
        lid = layer_ids[slot]
        color_out = create_layer_diffuse(slot, lid, 200)
        if color_out:
            final_color = apply_color_map(color_out)
            links.new(final_color, bsdf.inputs['Base Color'])

        normal_out = create_layer_normal(slot, lid, 200)
        if normal_out:
            normal_map = nodes.new('ShaderNodeNormalMap')
            normal_map.location = (2700, -200)
            links.new(normal_out, normal_map.inputs['Color'])
            links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])
        return mat

    blend_tex = nodes.new('ShaderNodeTexImage')
    blend_tex.location = (-1400, -700)
    blend_tex.image = blend_img
    blend_tex.interpolation = 'Linear'
    blend_tex.extension = 'EXTEND'
    links.new(uv_blend.outputs['UV'], blend_tex.inputs['Vector'])

    sep_color = nodes.new('ShaderNodeSeparateColor')
    sep_color.location = (-1100, -700)
    sep_color.mode = 'RGB'
    links.new(blend_tex.outputs['Color'], sep_color.inputs['Color'])

    weights_raw = [
        sep_color.outputs['Red'],
        sep_color.outputs['Green'],
        sep_color.outputs['Blue'],
        blend_tex.outputs['Alpha'],
    ]

    add1 = nodes.new('ShaderNodeMath')
    add1.operation = 'ADD'
    add1.location = (-800, -800)
    links.new(weights_raw[0], add1.inputs[0])
    links.new(weights_raw[1], add1.inputs[1])

    add2 = nodes.new('ShaderNodeMath')
    add2.operation = 'ADD'
    add2.location = (-600, -800)
    links.new(add1.outputs[0], add2.inputs[0])
    links.new(weights_raw[2], add2.inputs[1])

    add3 = nodes.new('ShaderNodeMath')
    add3.operation = 'ADD'
    add3.location = (-400, -800)
    links.new(add2.outputs[0], add3.inputs[0])
    links.new(weights_raw[3], add3.inputs[1])

    sum_safe = nodes.new('ShaderNodeMath')
    sum_safe.operation = 'MAXIMUM'
    sum_safe.location = (-200, -800)
    links.new(add3.outputs[0], sum_safe.inputs[0])
    sum_safe.inputs[1].default_value = 0.001

    norm_weights = []
    for i in range(4):
        div = nodes.new('ShaderNodeMath')
        div.operation = 'DIVIDE'
        div.location = (100, -500 - i * 80)
        links.new(weights_raw[i], div.inputs[0])
        links.new(sum_safe.outputs[0], div.inputs[1])
        norm_weights.append(div.outputs[0])

    layer_diffuse = []
    layer_normal = []
    for i in range(4):
        lid = layer_ids[i]
        y_off = 600 - i * 300

        if lid != 0 and f"layer_{lid}_diffuse" in layer_images:
            diff_out = create_layer_diffuse(i, lid, y_off)
            if diff_out:
                layer_diffuse.append(diff_out)
            else:
                white = nodes.new('ShaderNodeRGB')
                white.location = (-1000, y_off)
                white.outputs[0].default_value = (1, 1, 1, 1)
                layer_diffuse.append(white.outputs[0])

            norm_out = create_layer_normal(i, lid, y_off)
            if norm_out:
                layer_normal.append(norm_out)
            else:
                flat = nodes.new('ShaderNodeRGB')
                flat.location = (-600, y_off)
                flat.outputs[0].default_value = (0.5, 0.5, 1.0, 1.0)
                layer_normal.append(flat.outputs[0])
        else:
            white = nodes.new('ShaderNodeRGB')
            white.location = (-1000, y_off)
            white.outputs[0].default_value = (1, 1, 1, 1)
            layer_diffuse.append(white.outputs[0])

            flat = nodes.new('ShaderNodeRGB')
            flat.location = (-600, y_off)
            flat.outputs[0].default_value = (0.5, 0.5, 1.0, 1.0)
            layer_normal.append(flat.outputs[0])

    weighted_diffuse = []
    for i in range(4):
        mult = nodes.new('ShaderNodeVectorMath')
        mult.operation = 'SCALE'
        mult.location = (500, 600 - i * 200)
        links.new(layer_diffuse[i], mult.inputs[0])
        links.new(norm_weights[i], mult.inputs['Scale'])
        weighted_diffuse.append(mult.outputs[0])

    weighted_normal = []
    for i in range(4):
        mult = nodes.new('ShaderNodeVectorMath')
        mult.operation = 'SCALE'
        mult.location = (500, -400 - i * 200)
        links.new(layer_normal[i], mult.inputs[0])
        links.new(norm_weights[i], mult.inputs['Scale'])
        weighted_normal.append(mult.outputs[0])

    dsum1 = nodes.new('ShaderNodeVectorMath')
    dsum1.operation = 'ADD'
    dsum1.location = (800, 500)
    links.new(weighted_diffuse[0], dsum1.inputs[0])
    links.new(weighted_diffuse[1], dsum1.inputs[1])

    dsum2 = nodes.new('ShaderNodeVectorMath')
    dsum2.operation = 'ADD'
    dsum2.location = (1000, 400)
    links.new(dsum1.outputs[0], dsum2.inputs[0])
    links.new(weighted_diffuse[2], dsum2.inputs[1])

    dsum3 = nodes.new('ShaderNodeVectorMath')
    dsum3.operation = 'ADD'
    dsum3.location = (1200, 300)
    links.new(dsum2.outputs[0], dsum3.inputs[0])
    links.new(weighted_diffuse[3], dsum3.inputs[1])

    nsum1 = nodes.new('ShaderNodeVectorMath')
    nsum1.operation = 'ADD'
    nsum1.location = (800, -500)
    links.new(weighted_normal[0], nsum1.inputs[0])
    links.new(weighted_normal[1], nsum1.inputs[1])

    nsum2 = nodes.new('ShaderNodeVectorMath')
    nsum2.operation = 'ADD'
    nsum2.location = (1000, -600)
    links.new(nsum1.outputs[0], nsum2.inputs[0])
    links.new(weighted_normal[2], nsum2.inputs[1])

    nsum3 = nodes.new('ShaderNodeVectorMath')
    nsum3.operation = 'ADD'
    nsum3.location = (1200, -700)
    links.new(nsum2.outputs[0], nsum3.inputs[0])
    links.new(weighted_normal[3], nsum3.inputs[1])

    final_diffuse = apply_color_map(dsum3.outputs[0])
    links.new(final_diffuse, bsdf.inputs['Base Color'])

    normal_map = nodes.new('ShaderNodeNormalMap')
    normal_map.location = (2700, -400)
    links.new(nsum3.outputs[0], normal_map.inputs['Color'])
    links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])

    return mat


def create_mesh_object(chunk_name, chunk_data, world_offset=(0, 0, 0)):
    positions = chunk_data.get('positions', [])
    normals = chunk_data.get('normals', [])
    uvs = chunk_data.get('uvs', [])
    indices = chunk_data.get('indices', [])

    if not positions or not indices:
        return None

    mesh = bpy.data.meshes.new(chunk_name)
    obj = bpy.data.objects.new(chunk_name, mesh)

    verts = []
    num_verts = len(positions) // 3
    for i in range(num_verts):
        x = positions[i * 3] - world_offset[0]
        y = positions[i * 3 + 1] - world_offset[1]
        z = positions[i * 3 + 2] - world_offset[2]
        verts.append((x, -z, y))

    faces = []
    num_tris = len(indices) // 3
    for i in range(num_tris):
        faces.append((indices[i * 3], indices[i * 3 + 2], indices[i * 3 + 1]))

    mesh.from_pydata(verts, [], faces)

    mesh.uv_layers.new(name="UVMap")
    uv_layer = mesh.uv_layers.active.data

    for poly in mesh.polygons:
        for loop_idx in poly.loop_indices:
            vert_idx = mesh.loops[loop_idx].vertex_index
            if vert_idx < len(uvs) // 2:
                u = uvs[vert_idx * 2]
                v = uvs[vert_idx * 2 + 1]
                uv_layer[loop_idx].uv = (u, v)

    custom_normals = []
    for i in range(num_verts):
        if i < len(normals) // 3:
            nx = normals[i * 3]
            ny = normals[i * 3 + 1]
            nz = normals[i * 3 + 2]
            custom_normals.append((nx, -nz, ny))
        else:
            custom_normals.append((0, 0, 1))

    mesh.normals_split_custom_set_from_vertices(custom_normals)
    mesh.update()

    return obj


def import_terrain(context, filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    terrain_name = data.get('name', 'Terrain')
    layers_data = data.get('layers', {})
    chunks_data = data.get('chunks', [])

    collection = bpy.data.collections.new(terrain_name)
    context.scene.collection.children.link(collection)

    all_layer_ids = set()
    for chunk_data in chunks_data:
        for lid in chunk_data.get('layerIds', []):
            if lid != 0:
                all_layer_ids.add(lid)

    fallback_diffuse = None
    fallback_normal = None

    layer_images = {}
    for layer_id, layer_info in layers_data.items():
        if 'diffuse' in layer_info and 'data' in layer_info['diffuse']:
            img = load_image_from_base64(
                f"layer_{layer_id}_diffuse",
                layer_info['diffuse']['data']
            )
            layer_images[f"layer_{layer_id}_diffuse"] = img

        if 'normal' in layer_info and 'data' in layer_info['normal']:
            img = load_image_from_base64(
                f"layer_{layer_id}_normal",
                layer_info['normal']['data']
            )
            layer_images[f"layer_{layer_id}_normal"] = img

    for lid in all_layer_ids:
        if f"layer_{lid}_diffuse" not in layer_images:
            if fallback_diffuse is None:
                fallback_diffuse = create_solid_color_image("fallback_diffuse", 255, 0, 255)
            layer_images[f"layer_{lid}_diffuse"] = fallback_diffuse
            print(f"WARNING: Layer {lid} missing diffuse texture, using magenta fallback")

        if f"layer_{lid}_normal" not in layer_images:
            if fallback_normal is None:
                fallback_normal = create_solid_color_image("fallback_normal", 128, 128, 255)
            layer_images[f"layer_{lid}_normal"] = fallback_normal

    blend_images = {}
    for chunk_data in chunks_data:
        chunk_idx = chunk_data.get('index', 0)
        if 'blendMap' in chunk_data and 'data' in chunk_data['blendMap']:
            img = load_image_from_base64(
                f"chunk_{chunk_idx}_blend",
                chunk_data['blendMap']['data']
            )
            blend_images[f"chunk_{chunk_idx}_blend"] = img

    color_images = {}
    for chunk_data in chunks_data:
        chunk_idx = chunk_data.get('index', 0)
        if 'colorMap' in chunk_data and 'data' in chunk_data['colorMap']:
            img = load_image_from_base64(
                f"chunk_{chunk_idx}_color",
                chunk_data['colorMap']['data'],
                flip_v=True
            )
            color_images[f"chunk_{chunk_idx}_color"] = img

    world_offset = (0, 0, 0)
    if chunks_data:
        first_positions = chunks_data[0].get('positions', [])
        if len(first_positions) >= 3:
            world_offset = (first_positions[0], first_positions[1], first_positions[2])

    for chunk_data in chunks_data:
        chunk_idx = chunk_data.get('index', 0)
        chunk_name = f"Chunk_{chunk_idx}"

        obj = create_mesh_object(chunk_name, chunk_data, world_offset)
        if obj:
            collection.objects.link(obj)
            mat = create_splatmap_material(
                chunk_name, chunk_data, layers_data, layer_images, blend_images, color_images
            )
            obj.data.materials.append(mat)

    return {'FINISHED'}


if __name__ == "__main__":
    register()