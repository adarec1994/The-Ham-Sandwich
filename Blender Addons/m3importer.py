bl_info = {
    "name": "WildStar M3 Importer",
    "description": "Import WildStar (.m3) model files with skeleton, mesh, skinning, and animations",
    "author": "Matthew",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "category": "Import-Export",
    "location": "File > Import > WildStar M3 (.m3)",
}

import bpy
import os
import struct
import math
from mathutils import Matrix, Vector, Quaternion
from bpy.props import StringProperty, BoolProperty, IntProperty, EnumProperty, FloatProperty
from bpy_extras.io_utils import ImportHelper
from bpy.types import Operator


HEADER_SIZE = 1584
BONE_SIZE = 352
GEOM_SIZE = 208
SUBMESH_SIZE = 112
ANIMATION_SIZE = 112


def _u8(data, o):  return data[o]
def _u16(data, o): return struct.unpack_from("<H", data, o)[0]
def _s16(data, o): return struct.unpack_from("<h", data, o)[0]
def _u32(data, o): return struct.unpack_from("<I", data, o)[0]
def _s32(data, o): return struct.unpack_from("<i", data, o)[0]
def _u64(data, o): return struct.unpack_from("<Q", data, o)[0]
def _s64(data, o): return struct.unpack_from("<q", data, o)[0]
def _f32(data, o): return struct.unpack_from("<f", data, o)[0]


def _meta(data, o):
    return (_s64(data, o), _s64(data, o + 8))


def half_to_float(h):
    s = (h & 0x8000) << 16
    e = (h & 0x7C00) >> 10
    m = (h & 0x03FF) << 13
    if e == 0:
        v = s if m == 0 else (s | 0x00800000 | (m << 1))
    elif e == 0x1F:
        v = s | 0x7F800000 | m
    else:
        v = s | ((e + 112) << 23) | m
    return struct.unpack("<f", struct.pack("<I", v))[0]


def int16_norm(v):
    return v / 16383.5


def read_matrix(data, ofs):
    fl = struct.unpack_from("<16f", data, ofs)
    return Matrix((
        (fl[0], fl[4], fl[8],  fl[12]),
        (fl[1], fl[5], fl[9],  fl[13]),
        (fl[2], fl[6], fl[10], fl[14]),
        (fl[3], fl[7], fl[11], fl[15]),
    ))


def read_vertex_v3(data, base, offset, field_type):
    if field_type == 1:
        x = _f32(data, base + offset)
        y = _f32(data, base + offset + 4)
        z = _f32(data, base + offset + 8)
        return Vector((x, y, z)), 12
    elif field_type == 2:
        x = _s16(data, base + offset) / 1024.0
        y = _s16(data, base + offset + 2) / 1024.0
        z = _s16(data, base + offset + 4) / 1024.0
        return Vector((x, y, z)), 6
    elif field_type == 3:
        bx = _u8(data, base + offset)
        by = _u8(data, base + offset + 1)
        fx = (bx - 127.0) / 127.0
        fy = (by - 127.0) / 127.0
        fz = math.sqrt(max(1.0 - fx * fx - fy * fy, 0.0))
        return Vector((fx, fy, fz)), 2
    return Vector((0.0, 0.0, 0.0)), 0


def read_vertex_v4(data, base, offset, field_type):
    if field_type == 4:
        a = _u8(data, base + offset)
        b = _u8(data, base + offset + 1)
        c = _u8(data, base + offset + 2)
        d = _u8(data, base + offset + 3)
        return (a, b, c, d), 4
    return (0, 0, 0, 0), 0


def read_vertex_v2(data, base, offset, field_type):
    if field_type == 5:
        u = half_to_float(_u16(data, base + offset))
        v = half_to_float(_u16(data, base + offset + 2))
        return Vector((u, v)), 4
    return Vector((0.0, 0.0)), 0


def parse_track(data, anim_start, track):
    if track["duration"] <= 0:
        return
    dur = track["duration"]
    time_ofs = anim_start + track["time_offset"]
    val_ofs = anim_start + track["value_offset"]
    t_type = track["type"]
    kfs = []
    if 1 <= t_type <= 3:
        for i in range(dur):
            ts = _u32(data, time_ofs + i * 4)
            h = struct.unpack_from("<3H", data, val_ofs + i * 6)
            sx = half_to_float(h[0])
            sy = half_to_float(h[1])
            sz = half_to_float(h[2])
            kfs.append({"t": ts, "scale": Vector((sx, sy, sz))})
    elif t_type == 5 or t_type == 6:
        for i in range(dur):
            ts = _u32(data, time_ofs + i * 4)
            q = struct.unpack_from("<4h", data, val_ofs + i * 8)
            rw = int16_norm(q[3])
            rx = int16_norm(q[0])
            ry = int16_norm(q[1])
            rz = int16_norm(q[2])
            kfs.append({"t": ts, "rot": Quaternion((rw, rx, ry, rz))})
    elif t_type == 7:
        for i in range(dur):
            ts = _u32(data, time_ofs + i * 4)
            tx = _f32(data, val_ofs + i * 12)
            ty = _f32(data, val_ofs + i * 12 + 4)
            tz = _f32(data, val_ofs + i * 12 + 8)
            kfs.append({"t": ts, "trans": Vector((tx, ty, tz))})
    track["keyframes"] = kfs


def parse_bones(data, meta, model):
    count, offset = meta
    if count <= 0:
        return
    table_start = HEADER_SIZE + offset
    anim_start = table_start + count * BONE_SIZE
    bones = []
    for i in range(count):
        ofs = table_start + i * BONE_SIZE
        if ofs + BONE_SIZE > len(data):
            break
        bone = {
            "id": i,
            "name": f"Bone_{i}",
            "global_id": _s16(data, ofs),
            "flags": _u16(data, ofs + 2),
            "parent_id": _s16(data, ofs + 4),
            "tracks": [],
        }
        tofs = ofs + 16
        for t in range(8):
            tr = {
                "duration": _s64(data, tofs),
                "time_offset": _s64(data, tofs + 8),
                "value_offset": _s64(data, tofs + 16),
                "type": t + 1,
                "keyframes": [],
            }
            bone["tracks"].append(tr)
            tofs += 24
        bone["global_matrix"] = read_matrix(data, ofs + 0xD0)
        bone["inverse_global_matrix"] = read_matrix(data, ofs + 0x110)
        for t in range(8):
            parse_track(data, anim_start, bone["tracks"][t])
        bones.append(bone)
    model["bones"] = bones


def parse_bone_mapping_lut(data, meta, model):
    count, offset = meta
    if count <= 0:
        return
    start = HEADER_SIZE + offset
    if start + count * 2 > len(data):
        return
    model["lut_bone_mapping"] = list(struct.unpack_from(f"<{count}h", data, start))


def parse_geometry(data, meta, model):
    _, offset = meta
    if offset == 0:
        return
    gofs = HEADER_SIZE + offset
    if gofs + GEOM_SIZE > len(data):
        return
    nr_vertices = _u32(data, gofs + 0x18)
    vertex_size = _u16(data, gofs + 0x1C)
    vertex_flags = _s16(data, gofs + 0x1E)
    field_types = list(data[gofs + 0x20 : gofs + 0x20 + 11])
    nr_indices = _u32(data, gofs + 0x68)
    index_flags = _s16(data, gofs + 0x6C)
    ofs_indices = _u32(data, gofs + 0x78)
    nr_submeshes = _u32(data, gofs + 0x80)
    ofs_submeshes = _u32(data, gofs + 0x88)

    vertex_start = gofs + GEOM_SIZE
    vertices = []
    for i in range(nr_vertices):
        vbase = vertex_start + i * vertex_size
        local = 0
        pos = Vector((0.0, 0.0, 0.0))
        normal = Vector((0.0, 0.0, 1.0))
        bi = (0, 0, 0, 0)
        bw = (255, 0, 0, 0)
        col = (255, 255, 255, 255)
        blend = (0, 0, 0, 0)
        uv1 = Vector((0.0, 0.0))
        uv2 = Vector((0.0, 0.0))
        if vertex_flags & 0x0001:
            v, c = read_vertex_v3(data, vbase, local, field_types[0])
            pos = v; local += c
        if vertex_flags & 0x0002:
            _, c = read_vertex_v3(data, vbase, local, field_types[1])
            local += c
        if vertex_flags & 0x0004:
            v, c = read_vertex_v3(data, vbase, local, field_types[2])
            normal = v; local += c
        if vertex_flags & 0x0008:
            _, c = read_vertex_v3(data, vbase, local, field_types[3])
            local += c
        if vertex_flags & 0x0010:
            v, c = read_vertex_v4(data, vbase, local, field_types[4])
            bi = v; local += c
        if vertex_flags & 0x0020:
            v, c = read_vertex_v4(data, vbase, local, field_types[5])
            bw = v; local += c
        if vertex_flags & 0x0040:
            v, c = read_vertex_v4(data, vbase, local, field_types[6])
            col = v; local += c
        if vertex_flags & 0x0080:
            v, c = read_vertex_v4(data, vbase, local, field_types[7])
            blend = v; local += c
        if vertex_flags & 0x0100:
            v, c = read_vertex_v2(data, vbase, local, field_types[8])
            uv1 = v; local += c
        if vertex_flags & 0x0200:
            v, c = read_vertex_v2(data, vbase, local, field_types[9])
            uv2 = v; local += c
        vertices.append({
            "pos": pos,
            "normal": normal,
            "bone_indices": list(bi),
            "bone_weights": [w / 255.0 for w in bw],
            "uv1": uv1,
            "uv2": uv2,
        })

    index_start = gofs + GEOM_SIZE + ofs_indices
    is32 = (index_flags & 0x0200) == 0x0200
    if is32:
        indices = list(struct.unpack_from(f"<{nr_indices}I", data, index_start))
    else:
        indices = list(struct.unpack_from(f"<{nr_indices}H", data, index_start))

    submesh_start = gofs + GEOM_SIZE + ofs_submeshes
    submeshes = []
    for i in range(nr_submeshes):
        so = submesh_start + i * SUBMESH_SIZE
        sm = {
            "start_index": _u32(data, so),
            "start_vertex": _u32(data, so + 4),
            "index_count": _u32(data, so + 8),
            "vertex_count": _u32(data, so + 12),
            "start_bone_mapping": _u16(data, so + 16),
            "nr_bone_mapping": _u16(data, so + 18),
            "material_id": _u16(data, so + 22),
            "group_id": _u8(data, so + 30),
        }
        submeshes.append(sm)

    model["vertices"] = vertices
    model["indices"] = indices
    model["submeshes"] = submeshes


def parse_animations(data, meta, model):
    count, offset = meta
    if count <= 0:
        return
    table_start = HEADER_SIZE + offset
    anims = []
    for i in range(count):
        ofs = table_start + i * ANIMATION_SIZE
        if ofs + ANIMATION_SIZE > len(data):
            break
        anims.append({
            "sequence_id": _u16(data, ofs),
            "timestamp_start": _u32(data, ofs + 12),
            "timestamp_end": _u32(data, ofs + 16),
        })
    model["animations"] = anims


def apply_bone_mapping(model):
    lut = model.get("lut_bone_mapping", [])
    if not lut:
        return
    vertices = model.get("vertices", [])
    for sm in model.get("submeshes", []):
        submap = []
        for j in range(sm["start_bone_mapping"], sm["start_bone_mapping"] + sm["nr_bone_mapping"]):
            if 0 <= j < len(lut):
                submap.append(lut[j])
        if not submap:
            continue
        for vi in range(sm["start_vertex"], sm["start_vertex"] + sm["vertex_count"]):
            if vi >= len(vertices):
                break
            v = vertices[vi]
            for k in range(4):
                if v["bone_weights"][k] > 0.0 and v["bone_indices"][k] < len(submap):
                    v["bone_indices"][k] = submap[v["bone_indices"][k]]


def fix_mirrored_bones(model):
    bones = model["bones"]
    mirrored_structural = set()
    for i, b in enumerate(bones):
        gm = b["global_matrix"]
        if 0 <= b["parent_id"] < len(bones):
            local = bones[b["parent_id"]]["inverse_global_matrix"] @ gm
        else:
            local = gm
        if local.to_3x3().determinant() < 0:
            mirrored_structural.add(i)
    mirrored_anim = set()
    for i, b in enumerate(bones):
        for t in (0, 1):
            if b["tracks"][t]["keyframes"]:
                s = b["tracks"][t]["keyframes"][0]["scale"]
                if s.x < 0:
                    mirrored_anim.add(i)
    for bi in mirrored_anim:
        if bi not in mirrored_structural:
            b = bones[bi]
            for kf in b["tracks"][0]["keyframes"]:
                kf["scale"] = Vector((-kf["scale"].x, -kf["scale"].y, -kf["scale"].z))


def parse_m3(data):
    if len(data) < HEADER_SIZE:
        raise ValueError("File too small to be a valid .m3")
    sig = bytes(data[0:4])
    version = _u32(data, 4)
    if version != 100:
        raise ValueError(f"Unsupported M3 version {version} (only 100 supported; older v90-99 not handled by this importer)")
    animations_meta = _meta(data, 0x10)
    bones_meta = _meta(data, 0x180)
    lut_bone_ids_meta = _meta(data, 0x1A0)
    geometry_meta = _meta(data, 0x250)
    model = {
        "bones": [],
        "vertices": [],
        "indices": [],
        "submeshes": [],
        "animations": [],
        "lut_bone_mapping": [],
    }
    parse_bones(data, bones_meta, model)
    parse_bone_mapping_lut(data, lut_bone_ids_meta, model)
    parse_geometry(data, geometry_meta, model)
    apply_bone_mapping(model)
    parse_animations(data, animations_meta, model)
    fix_mirrored_bones(model)
    return model


def interp_scale(track, ms):
    kfs = track["keyframes"]
    if not kfs: return None
    if len(kfs) == 1: return kfs[0]["scale"].copy()
    if ms <= kfs[0]["t"]: return kfs[0]["scale"].copy()
    if ms >= kfs[-1]["t"]: return kfs[-1]["scale"].copy()
    for k in range(len(kfs) - 1):
        if kfs[k]["t"] <= ms < kfs[k + 1]["t"]:
            a, b = kfs[k], kfs[k + 1]
            denom = float(b["t"] - a["t"])
            if denom <= 0.0: return a["scale"].copy()
            t = max(0.0, min(1.0, (ms - a["t"]) / denom))
            return a["scale"].lerp(b["scale"], t)
    return kfs[-1]["scale"].copy()


def interp_rotation(track, ms):
    kfs = track["keyframes"]
    if not kfs: return None
    if len(kfs) == 1: return kfs[0]["rot"].copy()
    if ms <= kfs[0]["t"]: return kfs[0]["rot"].copy()
    if ms >= kfs[-1]["t"]: return kfs[-1]["rot"].copy()
    for k in range(len(kfs) - 1):
        if kfs[k]["t"] <= ms < kfs[k + 1]["t"]:
            a, b = kfs[k], kfs[k + 1]
            denom = float(b["t"] - a["t"])
            if denom <= 0.0: return a["rot"].copy()
            t = max(0.0, min(1.0, (ms - a["t"]) / denom))
            q0 = a["rot"].normalized()
            q1 = b["rot"].normalized()
            if q0.dot(q1) < 0.0:
                q1 = Quaternion((-q1.w, -q1.x, -q1.y, -q1.z))
            return q0.slerp(q1, t)
    return kfs[-1]["rot"].copy()


def interp_translation(track, ms):
    kfs = track["keyframes"]
    if not kfs: return None
    if len(kfs) == 1: return kfs[0]["trans"].copy()
    if ms <= kfs[0]["t"]: return kfs[0]["trans"].copy()
    if ms >= kfs[-1]["t"]: return kfs[-1]["trans"].copy()
    for k in range(len(kfs) - 1):
        if kfs[k]["t"] <= ms < kfs[k + 1]["t"]:
            a, b = kfs[k], kfs[k + 1]
            denom = float(b["t"] - a["t"])
            if denom <= 0.0: return a["trans"].copy()
            t = max(0.0, min(1.0, (ms - a["t"]) / denom))
            return a["trans"].lerp(b["trans"], t)
    return kfs[-1]["trans"].copy()


def mat_translate(v):
    m = Matrix.Identity(4)
    m[0][3] = v.x; m[1][3] = v.y; m[2][3] = v.z
    return m


def mat_scale(v):
    m = Matrix.Identity(4)
    m[0][0] = v.x; m[1][1] = v.y; m[2][2] = v.z
    return m


def mat_rotation(q):
    return q.normalized().to_matrix().to_4x4()


def compute_effective_bind_global(bones):
    n = len(bones)
    eff = [Matrix.Identity(4) for _ in range(n)]
    for i, b in enumerate(bones):
        gm = b["global_matrix"]
        is_root = not (0 <= b["parent_id"] < n)
        at_origin = Vector((gm[0][3], gm[1][3], gm[2][3])).length < 0.001
        trk6 = b["tracks"][6]["keyframes"]
        if at_origin and trk6:
            t6 = trk6[0]["trans"]
            orig_det = gm.to_3x3().determinant()
            need_neg = orig_det < 0.0
            bind_scale = Vector((1.0, 1.0, 1.0))
            found = False
            for t in range(3):
                kfs = b["tracks"][t]["keyframes"]
                if kfs:
                    ts = kfs[0]["scale"]
                    neg = (ts.x * ts.y * ts.z) < 0.0
                    if neg == need_neg:
                        bind_scale = ts.copy(); found = True; break
            if not found:
                for t in range(3):
                    kfs = b["tracks"][t]["keyframes"]
                    if kfs:
                        bind_scale = kfs[0]["scale"].copy()
                        sd = bind_scale.x * bind_scale.y * bind_scale.z
                        if (sd < 0.0) != need_neg:
                            bind_scale.x = -bind_scale.x
                        break
            bind_rot = Quaternion((1.0, 0.0, 0.0, 0.0))
            for t in (4, 5):
                kfs = b["tracks"][t]["keyframes"]
                if kfs:
                    bind_rot = kfs[0]["rot"]; break
            local_t = mat_translate(t6) @ mat_rotation(bind_rot) @ mat_scale(bind_scale)
            if is_root:
                eff[i] = local_t
            else:
                eff[i] = eff[b["parent_id"]] @ local_t
        else:
            eff[i] = gm.copy()
    return eff


def precompute_bind_local(bones, eff):
    n = len(bones)
    mirrored = [False] * n
    bind_loc_scale = [Vector((1.0, 1.0, 1.0)) for _ in range(n)]
    bind_loc_rot = [Quaternion((1.0, 0.0, 0.0, 0.0)) for _ in range(n)]
    bind_loc_trans = [Vector((0.0, 0.0, 0.0)) for _ in range(n)]
    bone_at_origin = [False] * n
    for i, b in enumerate(bones):
        is_root = not (0 <= b["parent_id"] < n)
        gm = b["global_matrix"]
        at_origin = Vector((gm[0][3], gm[1][3], gm[2][3])).length < 0.001
        bone_at_origin[i] = at_origin
        if is_root:
            bl = eff[i]
        else:
            bl = eff[b["parent_id"]].inverted() @ eff[i]
        det = bl.to_3x3().determinant()
        mirrored[i] = det < 0.0
        t, r, s = bl.decompose()
        if mirrored[i]:
            if s.x * s.y * s.z > 0.0:
                s = Vector((-s.x, s.y, s.z))
                fixed_s = mat_scale(s)
                rpure = bl @ fixed_s.inverted()
                rpure[0][3] = 0.0; rpure[1][3] = 0.0; rpure[2][3] = 0.0
                _, r, _ = rpure.decompose()
        bind_loc_scale[i] = s
        bind_loc_rot[i] = r.normalized()
        if bind_loc_rot[i].w < 0.0:
            bind_loc_rot[i] = Quaternion((-r.w, -r.x, -r.y, -r.z))
        bind_loc_trans[i] = t
    return {
        "mirrored": mirrored,
        "scale": bind_loc_scale,
        "rot": bind_loc_rot,
        "trans": bind_loc_trans,
        "at_origin": bone_at_origin,
    }


def compute_world_at_time(bones, bind, ms):
    n = len(bones)
    world = [None] * n
    mirrored = bind["mirrored"]
    for i, b in enumerate(bones):
        bind_scale_det = bind["scale"][i].x * bind["scale"][i].y * bind["scale"][i].z
        need_neg = bind_scale_det < 0.0
        scale = bind["scale"][i].copy()
        found = False
        for t in range(3):
            kfs = b["tracks"][t]["keyframes"]
            if kfs:
                ts = interp_scale(b["tracks"][t], ms)
                td = ts.x * ts.y * ts.z
                if (td < 0.0) == need_neg:
                    scale = ts; found = True; break
        if not found:
            for t in range(3):
                kfs = b["tracks"][t]["keyframes"]
                if kfs:
                    scale = interp_scale(b["tracks"][t], ms)
                    sd = scale.x * scale.y * scale.z
                    if (sd < 0.0) != need_neg:
                        scale = Vector((-scale.x, scale.y, scale.z))
                    break
        rotation = bind["rot"][i].copy()
        for t in (4, 5):
            kfs = b["tracks"][t]["keyframes"]
            if kfs:
                rotation = interp_rotation(b["tracks"][t], ms); break
        translation = bind["trans"][i].copy()
        if bind["at_origin"][i]:
            trk6 = b["tracks"][6]["keyframes"]
            if trk6:
                translation = interp_translation(b["tracks"][6], ms)
        T = mat_translate(translation)
        R = mat_rotation(rotation)
        S = mat_scale(scale)
        if mirrored[i]:
            local = T @ S @ R
        else:
            local = T @ R @ S
        if not (0 <= b["parent_id"] < n) or world[b["parent_id"]] is None:
            world[i] = local
        else:
            world[i] = world[b["parent_id"]] @ local
    return world


def build_armature(context, model, obj_name):
    bones = model["bones"]
    if not bones:
        return None, [], []
    eff = compute_effective_bind_global(bones)
    arm_data = bpy.data.armatures.new(obj_name + "_Armature")
    arm_obj = bpy.data.objects.new(obj_name + "_Armature", arm_data)
    context.collection.objects.link(arm_obj)
    context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')
    children = [[] for _ in bones]
    for i, b in enumerate(bones):
        if 0 <= b["parent_id"] < len(bones):
            children[b["parent_id"]].append(i)
    edit_names = []
    for i, b in enumerate(bones):
        head = Vector((eff[i][0][3], eff[i][1][3], eff[i][2][3]))
        if children[i]:
            child_head = Vector((eff[children[i][0]][0][3], eff[children[i][0]][1][3], eff[children[i][0]][2][3]))
            d = child_head - head
            if d.length < 1e-5:
                d = Vector((0.0, 0.05, 0.0))
            else:
                d = d.normalized() * max(0.02, min(d.length, 0.5))
        else:
            d = Vector((eff[i][0][1], eff[i][1][1], eff[i][2][1]))
            if d.length < 1e-5:
                d = Vector((0.0, 0.05, 0.0))
            else:
                d = d.normalized() * 0.05
        eb = arm_data.edit_bones.new(b["name"])
        eb.head = head
        eb.tail = head + d
        edit_names.append(eb.name)
    for i, b in enumerate(bones):
        if 0 <= b["parent_id"] < len(bones):
            arm_data.edit_bones[edit_names[i]].parent = arm_data.edit_bones[edit_names[b["parent_id"]]]
    bpy.ops.object.mode_set(mode='OBJECT')
    rest_matrices = [arm_data.bones[edit_names[i]].matrix_local.copy() for i in range(len(bones))]
    return arm_obj, edit_names, (eff, rest_matrices)


def build_mesh(context, model, arm_obj, edit_names, obj_name):
    vertices = model["vertices"]
    indices = model["indices"]
    if not vertices or not indices:
        return None
    verts_py = [tuple(v["pos"]) for v in vertices]
    faces_py = [(indices[i], indices[i + 1], indices[i + 2]) for i in range(0, len(indices) - 2, 3)]
    mesh = bpy.data.meshes.new(obj_name + "_Mesh")
    mesh_obj = bpy.data.objects.new(obj_name + "_Mesh", mesh)
    context.collection.objects.link(mesh_obj)
    mesh.from_pydata(verts_py, [], faces_py)
    mesh.update()
    if vertices[0]["uv1"] is not None:
        uv_layer = mesh.uv_layers.new(name="UVMap")
        for poly in mesh.polygons:
            for loop_idx in poly.loop_indices:
                vi = mesh.loops[loop_idx].vertex_index
                uv = vertices[vi]["uv1"]
                uv_layer.data[loop_idx].uv = (uv.x, 1.0 - uv.y)
    if vertices[0].get("normal") is not None:
        try:
            mesh.normals_split_custom_set_from_vertices([tuple(v["normal"]) for v in vertices])
            if hasattr(mesh, "use_auto_smooth"):
                mesh.use_auto_smooth = True
        except Exception:
            pass
    if arm_obj is not None:
        bones = model["bones"]
        vgroups = {}
        for i in range(len(bones)):
            vgroups[i] = mesh_obj.vertex_groups.new(name=edit_names[i])
        for vi, v in enumerate(vertices):
            for k in range(4):
                w = v["bone_weights"][k]
                if w <= 0.0:
                    continue
                bi = v["bone_indices"][k]
                if bi in vgroups:
                    vgroups[bi].add([vi], w, 'REPLACE')
        mod = mesh_obj.modifiers.new(name="Armature", type='ARMATURE')
        mod.object = arm_obj
        mesh_obj.parent = arm_obj
    mat = bpy.data.materials.new(obj_name + "_Mat")
    mat.use_nodes = True
    mesh.materials.append(mat)
    return mesh_obj


def bake_animation(context, model, arm_obj, anim_index, bind, eff, rest_matrices):
    bones = model["bones"]
    anim = model["animations"][anim_index]
    start_ms = anim["timestamp_start"]
    end_ms = anim["timestamp_end"]
    duration_ms = end_ms - start_ms
    if duration_ms <= 0:
        return None
    timestamps_set = set()
    for bi, b in enumerate(bones):
        for t in (0, 1, 2, 4, 5, 6):
            for kf in b["tracks"][t]["keyframes"]:
                if start_ms <= kf["t"] <= end_ms:
                    timestamps_set.add(kf["t"])
    timestamps_set.add(start_ms)
    timestamps_set.add(end_ms)
    if len(timestamps_set) < 2:
        return None
    timestamps = sorted(timestamps_set)
    fps = context.scene.render.fps / context.scene.render.fps_base
    action = bpy.data.actions.new(name=f"Anim_{anim['sequence_id']}")
    action.use_fake_user = True
    if arm_obj.animation_data is None:
        arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    pose_bones = arm_obj.pose.bones
    inv_eff = [m.inverted() for m in eff]
    inv_rest = [m.inverted() for m in rest_matrices]
    for pb in pose_bones:
        pb.rotation_mode = 'QUATERNION'
    for ts in timestamps:
        frame = 1.0 + ((ts - start_ms) / 1000.0) * fps
        world = compute_world_at_time(bones, bind, ts)
        desired = [None] * len(bones)
        for i in range(len(bones)):
            desired[i] = world[i] @ inv_eff[i] @ rest_matrices[i]
        for i, b in enumerate(bones):
            pb = pose_bones[arm_obj.data.bones[i].name]
            if 0 <= b["parent_id"] < len(bones) and desired[b["parent_id"]] is not None:
                parent_rest = rest_matrices[b["parent_id"]]
                parent_pose = desired[b["parent_id"]]
                basis = rest_matrices[i].inverted() @ parent_rest @ parent_pose.inverted() @ desired[i]
            else:
                basis = inv_rest[i] @ desired[i]
            loc, rot, scl = basis.decompose()
            pb.location = loc
            pb.rotation_quaternion = rot
            pb.scale = scl
            pb.keyframe_insert(data_path="location", frame=frame)
            pb.keyframe_insert(data_path="rotation_quaternion", frame=frame)
            pb.keyframe_insert(data_path="scale", frame=frame)
    for fcurve in action.fcurves:
        for kp in fcurve.keyframe_points:
            kp.interpolation = 'LINEAR'
    return action


def import_m3(context, filepath, import_animations, anim_limit):
    with open(filepath, 'rb') as f:
        raw = f.read()
    model = parse_m3(raw)
    base = os.path.splitext(os.path.basename(filepath))[0]
    arm_obj, edit_names, extras = build_armature(context, model, base)
    mesh_obj = None
    if arm_obj is not None:
        eff, rest_matrices = extras
        bind = precompute_bind_local(model["bones"], eff)
        mesh_obj = build_mesh(context, model, arm_obj, edit_names, base)
        if import_animations and model["animations"]:
            n_anims = len(model["animations"])
            count = n_anims if anim_limit <= 0 else min(anim_limit, n_anims)
            for i in range(count):
                try:
                    bake_animation(context, model, arm_obj, i, bind, eff, rest_matrices)
                except Exception as e:
                    print(f"[M3Import] Failed animation {i}: {e}")
    else:
        mesh_obj = build_mesh(context, model, None, [], base)
    return {
        "bones": len(model["bones"]),
        "vertices": len(model["vertices"]),
        "triangles": len(model["indices"]) // 3,
        "animations": len(model["animations"]),
    }


class IMPORT_OT_m3(Operator, ImportHelper):
    bl_idname = "import_scene.wildstar_m3"
    bl_label = "Import WildStar M3"
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".m3"
    filter_glob: StringProperty(default="*.m3", options={'HIDDEN'})
    import_animations: BoolProperty(name="Import Animations", default=True)
    anim_limit: IntProperty(name="Animation Count Limit", description="How many animations to import (0 = all). File may contain hundreds; start small.", default=10, min=0)

    def execute(self, context):
        try:
            stats = import_m3(context, self.filepath, self.import_animations, self.anim_limit)
            self.report({'INFO'}, f"Imported M3: {stats['bones']} bones, {stats['vertices']} verts, {stats['triangles']} tris, {stats['animations']} anims")
            return {'FINISHED'}
        except Exception as e:
            import traceback
            traceback.print_exc()
            self.report({'ERROR'}, f"Import failed: {e}")
            return {'CANCELLED'}


def menu_func_import(self, context):
    self.layout.operator(IMPORT_OT_m3.bl_idname, text="WildStar M3 (.m3)")


def register():
    bpy.utils.register_class(IMPORT_OT_m3)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.utils.unregister_class(IMPORT_OT_m3)


if __name__ == "__main__":
    register()