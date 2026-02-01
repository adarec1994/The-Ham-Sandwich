bl_info = {
    "name": "WildStar Terrain Importer",
    "author": "Matthew Wakeman",
    "version": (2, 4, 0),
    "blender": (3, 0, 0),
    "location": "File > Import > WildStar Terrain",
    "description": "Import WildStar terrain with props and skybox data",
    "category": "Import-Export",
}

import bpy
import json
import base64
import os
import tempfile
import math
from bpy.props import StringProperty, BoolProperty, FloatProperty
from bpy_extras.io_utils import ImportHelper
from mathutils import Vector, Quaternion, Matrix


class IMPORT_OT_wsterrain(bpy.types.Operator, ImportHelper):
    bl_idname = "import_scene.wsterrain"
    bl_label = "Import WildStar Terrain"
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = ".wsterrain"
    filter_glob: StringProperty(default="*.wsterrain", options={'HIDDEN'})

    import_props: BoolProperty(
        name="Import Props",
        description="Import prop instances from FBX files",
        default=True
    )

    import_terrain: BoolProperty(
        name="Import Terrain",
        description="Import terrain mesh chunks",
        default=True
    )

    scale_factor: FloatProperty(
        name="Scale",
        description="Scale factor for imported geometry",
        default=1.0,
        min=0.001,
        max=1000.0
    )

    def execute(self, context):
        return import_terrain(context, self.filepath, self.import_terrain, self.import_props, self.scale_factor)


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
        blend_tex.outputs['Alpha']
    ]

    add_weights = nodes.new('ShaderNodeMath')
    add_weights.operation = 'ADD'
    add_weights.location = (-800, -800)
    links.new(weights_raw[0], add_weights.inputs[0])
    links.new(weights_raw[1], add_weights.inputs[1])

    add_weights2 = nodes.new('ShaderNodeMath')
    add_weights2.operation = 'ADD'
    add_weights2.location = (-600, -800)
    links.new(add_weights.outputs[0], add_weights2.inputs[0])
    links.new(weights_raw[2], add_weights2.inputs[1])

    add_weights3 = nodes.new('ShaderNodeMath')
    add_weights3.operation = 'ADD'
    add_weights3.location = (-400, -800)
    links.new(add_weights2.outputs[0], add_weights3.inputs[0])
    links.new(weights_raw[3], add_weights3.inputs[1])

    clamp_total = nodes.new('ShaderNodeMath')
    clamp_total.operation = 'MAXIMUM'
    clamp_total.location = (-200, -800)
    clamp_total.inputs[1].default_value = 0.001
    links.new(add_weights3.outputs[0], clamp_total.inputs[0])

    norm_weights = []
    for i in range(4):
        div = nodes.new('ShaderNodeMath')
        div.operation = 'DIVIDE'
        div.location = (0, -600 - i * 100)
        links.new(weights_raw[i], div.inputs[0])
        links.new(clamp_total.outputs[0], div.inputs[1])
        norm_weights.append(div.outputs[0])

    layer_diffuse = []
    layer_normal = []

    for slot_i, slot in enumerate(range(4)):
        y_off = 600 - slot * 300
        lid = layer_ids[slot]
        if lid != 0:
            color_out = create_layer_diffuse(slot, lid, y_off)
            if color_out:
                layer_diffuse.append(color_out)
            else:
                white = nodes.new('ShaderNodeRGB')
                white.location = (-1000, y_off)
                white.outputs[0].default_value = (1, 1, 1, 1)
                layer_diffuse.append(white.outputs[0])

            normal_out = create_layer_normal(slot, lid, y_off)
            if normal_out:
                layer_normal.append(normal_out)
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


def create_mesh_object(chunk_name, chunk_data, scale_factor=1.0):
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
        x = positions[i * 3] * scale_factor
        y = positions[i * 3 + 1] * scale_factor
        z = positions[i * 3 + 2] * scale_factor
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


def duplicate_hierarchy(source_objects, collection):
    object_map = {}

    for src_obj in source_objects:
        new_obj = src_obj.copy()
        if src_obj.data:
            new_obj.data = src_obj.data
        object_map[src_obj] = new_obj
        collection.objects.link(new_obj)

    for src_obj, new_obj in object_map.items():
        if src_obj.parent and src_obj.parent in object_map:
            new_obj.parent = object_map[src_obj.parent]
            new_obj.matrix_parent_inverse = src_obj.matrix_parent_inverse.copy()

    root_objects = [new_obj for src_obj, new_obj in object_map.items() if src_obj.parent not in object_map]

    return root_objects[0] if root_objects else None


def import_prop_instance(terrain_dir, prop_data, props_collection, scale_factor, imported_models, models_path=None):
    model_name = prop_data.get('model', '')
    position = prop_data.get('position', [0, 0, 0])
    rotation = prop_data.get('rotation', [0, 0, 0, 1])
    prop_scale = prop_data.get('scale', 1.0)

    if not model_name:
        return None

    # Normalize path separators for the OS
    model_rel_path = model_name.replace('\\', os.sep).replace('/', os.sep)
    fbx_filename = model_rel_path + '.fbx'

    # Try multiple possible model paths
    possible_paths = []
    if models_path:
        possible_paths.append(os.path.join(models_path, fbx_filename))
    possible_paths.append(os.path.join(terrain_dir, 'models', fbx_filename))
    possible_paths.append(os.path.join(os.path.dirname(terrain_dir), 'models', fbx_filename))

    # Also try just the filename (legacy flat structure)
    base_filename = os.path.basename(model_rel_path) + '.fbx'
    if models_path:
        possible_paths.append(os.path.join(models_path, base_filename))
    possible_paths.append(os.path.join(terrain_dir, 'models', base_filename))
    possible_paths.append(os.path.join(os.path.dirname(terrain_dir), 'models', base_filename))

    fbx_path = None
    for path in possible_paths:
        if os.path.exists(path):
            fbx_path = path
            break

    if not fbx_path:
        print(f"FBX not found for '{model_name}'. Tried: {possible_paths[:3]}")
        return None

    if model_name in imported_models:
        cached = imported_models[model_name]
        if cached is None:
            return None

        root_obj = duplicate_hierarchy(cached['objects'], props_collection)
        if root_obj is None:
            return None
    else:
        try:
            old_objects = set(bpy.data.objects)

            # Try native FBX importer first (Blender 4.2+), fallback to Python addon
            try:
                bpy.ops.wm.fbx_import(filepath=fbx_path)
            except AttributeError:
                bpy.ops.import_scene.fbx(filepath=fbx_path)

            new_objects = [o for o in bpy.data.objects if o not in old_objects]

            if not new_objects:
                imported_models[model_name] = None
                return None

            for obj in new_objects:
                for coll in list(obj.users_collection):
                    coll.objects.unlink(obj)
                props_collection.objects.link(obj)

            root_obj = None
            for obj in new_objects:
                if obj.parent is None or obj.parent not in new_objects:
                    root_obj = obj
                    break

            if root_obj is None:
                root_obj = new_objects[0]

            imported_models[model_name] = {
                'objects': new_objects,
                'root': root_obj
            }

        except Exception as e:
            print(f"Error importing FBX {fbx_path}: {e}")
            imported_models[model_name] = None
            return None

    if root_obj is None:
        return None

    blender_pos = Vector((
        position[0] * scale_factor,
        -position[2] * scale_factor,
        position[1] * scale_factor
    ))

    ws_quat = Quaternion((rotation[3], rotation[0], rotation[1], rotation[2]))
    coord_fix = Quaternion((0.707107, 0.707107, 0, 0))
    blender_rot = coord_fix @ ws_quat

    root_obj.location = blender_pos
    root_obj.rotation_mode = 'QUATERNION'
    root_obj.rotation_quaternion = blender_rot
    root_obj.scale = (prop_scale * scale_factor, prop_scale * scale_factor, prop_scale * scale_factor)

    return root_obj


def import_terrain(context, filepath, import_terrain_mesh=True, import_props=True, scale_factor=1.0):
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    terrain_dir = os.path.dirname(filepath)
    terrain_name = data.get('name', 'Terrain')
    layers_data = data.get('layers', {})
    chunks_data = data.get('chunks', [])
    props_data = data.get('props', [])
    skyboxes = data.get('skyboxes', [])

    main_collection = bpy.data.collections.new(terrain_name)
    context.scene.collection.children.link(main_collection)

    terrain_collection = bpy.data.collections.new(f"{terrain_name}_Terrain")
    main_collection.children.link(terrain_collection)

    props_collection = bpy.data.collections.new(f"{terrain_name}_Props")
    main_collection.children.link(props_collection)

    if import_terrain_mesh:
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

        for chunk_data in chunks_data:
            chunk_idx = chunk_data.get('index', 0)
            chunk_name = f"Chunk_{chunk_idx}"

            obj = create_mesh_object(chunk_name, chunk_data, scale_factor)
            if obj:
                terrain_collection.objects.link(obj)
                mat = create_splatmap_material(
                    chunk_name, chunk_data, layers_data, layer_images, blend_images, color_images
                )
                obj.data.materials.append(mat)

        print(f"Imported {len(chunks_data)} terrain chunks")

    if import_props and props_data:
        imported_models = {}

        # Detect models folder - try multiple locations
        models_path = None
        possible_models_dirs = [
            os.path.join(terrain_dir, 'models'),
            os.path.join(os.path.dirname(terrain_dir), 'models'),
        ]
        for mdir in possible_models_dirs:
            if os.path.isdir(mdir):
                models_path = mdir
                break

        if models_path:
            print(f"Found models folder: {models_path}")
        else:
            print(f"WARNING: No models folder found. Tried: {possible_models_dirs}")

        for i, prop_data in enumerate(props_data):
            import_prop_instance(terrain_dir, prop_data, props_collection, scale_factor, imported_models, models_path)

        print(f"Imported {len(props_data)} prop instances from {len(imported_models)} unique models")

    if skyboxes:
        main_collection["skybox_ids"] = skyboxes
        print(f"Terrain uses {len(skyboxes)} unique skybox IDs: {skyboxes}")

    return {'FINISHED'}


if __name__ == "__main__":
    register()