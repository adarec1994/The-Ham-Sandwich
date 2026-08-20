using System;
using System.Buffers.Binary;
using Godot;

namespace WildStar.Model;

public static class M3MeshBuilder
{
    public static bool TryBuild(M3File model, out ArrayMesh mesh, out string error)
    {
        mesh = null!;

        if (!model.Fields[(int)M3Slot.Position].Present)
        {
            error = "vertex format declares no position";
            return false;
        }

        if (model.IndexCount < 3)
        {
            error = "no index data";
            return false;
        }

        return TryBuild(model, out mesh, out _, out error);
    }

    public static bool TryBuild(M3File model, out ArrayMesh mesh, out int[] surfaceGeosets,
                                out string error)
        => TryBuild(model, out mesh, out surfaceGeosets, out _, out error);

    public static bool TryBuild(M3File model, out ArrayMesh mesh, out int[] surfaceGeosets,
                                out int[] surfaceMaterials, out string error)
    {
        mesh = new ArrayMesh();
        surfaceGeosets = System.Array.Empty<int>();
        surfaceMaterials = System.Array.Empty<int>();

        var geosets = new System.Collections.Generic.List<int>();
        var materials = new System.Collections.Generic.List<int>();

        M3Submesh[] submeshes = model.Submeshes.Length != 0
            ? model.Submeshes
            : new[] { new M3Submesh(0, 0, model.IndexCount, model.VertexCount, 0, 0,
                                    model.BoneMap.Length, 0, 0xFFFF, 0, 0, 0,
                                    new ushort[4], new ushort[4]) };

        foreach (M3Submesh submesh in submeshes)
        {
            int before = mesh.GetSurfaceCount();
            if (!AddSurface(mesh, model, submesh, out error))
            {
                return false;
            }

            if (mesh.GetSurfaceCount() > before)
            {
                geosets.Add(submesh.GeosetId);
                materials.Add(submesh.MaterialIndex);
            }
        }

        if (mesh.GetSurfaceCount() == 0)
        {
            error = "no usable submesh";
            return false;
        }

        surfaceGeosets = geosets.ToArray();
        surfaceMaterials = materials.ToArray();
        error = string.Empty;
        return true;
    }

    private static bool AddSurface(ArrayMesh mesh, M3File model, M3Submesh submesh, out string error)
    {
        error = string.Empty;

        int triangles = submesh.IndexCount / 3;
        if (triangles == 0 || submesh.VertexCount == 0)
        {
            return true;
        }

        if (submesh.StartVertex + submesh.VertexCount > model.VertexCount ||
            submesh.StartIndex + submesh.IndexCount > model.IndexCount)
        {
            error = "submesh runs past the vertex or index buffer";
            return false;
        }

        M3VertexField position = model.Fields[(int)M3Slot.Position];
        M3VertexField normal = model.Fields[(int)M3Slot.Normal];
        M3VertexField tangent = model.Fields[(int)M3Slot.Tangent];
        M3VertexField bitangent = model.Fields[(int)M3Slot.Bitangent];
        M3VertexField uv = model.Fields[(int)M3Slot.Uv0].Present
            ? model.Fields[(int)M3Slot.Uv0]
            : model.Fields[(int)M3Slot.Uv1];
        M3VertexField uv2 = model.Fields[(int)M3Slot.Uv1];
        M3VertexField boneIndices = model.Fields[(int)M3Slot.BoneIndices];
        M3VertexField boneWeights = model.Fields[(int)M3Slot.BoneWeights];

        int count = submesh.VertexCount;

        var vertices = new Vector3[count];
        var normals = new Vector3[count];
        var uvs = new Vector2[count];
        var uvs2 = new Vector2[count];
        var tangents = new float[count * 4];

        bool skinned = boneIndices.Present && model.Bones.Length != 0;
        var bones = skinned ? new int[count * 4] : Array.Empty<int>();
        var weights = skinned ? new float[count * 4] : Array.Empty<float>();

        for (int i = 0; i < count; i++)
        {
            ReadOnlySpan<byte> vertex = model.Vertex(submesh.StartVertex + i);

            vertices[i] = ReadPosition(vertex, position);
            normals[i] = normal.Present ? DecodeOct(vertex, normal.Offset) : Vector3.Up;

            uvs[i] = uv.Present ? ReadHalf2(vertex, uv.Offset) : Vector2.Zero;
            uvs2[i] = uv2.Present ? ReadHalf2(vertex, uv2.Offset) : uvs[i];

            if (tangent.Present)
            {
                Vector3 t = DecodeOct(vertex, tangent.Offset);
                float sign = 1.0f;

                if (bitangent.Present)
                {
                    Vector3 b = DecodeOct(vertex, bitangent.Offset);
                    sign = normals[i].Cross(t).Dot(b) < 0.0f ? -1.0f : 1.0f;
                }

                tangents[i * 4] = t.X;
                tangents[i * 4 + 1] = t.Y;
                tangents[i * 4 + 2] = t.Z;
                tangents[i * 4 + 3] = sign;
            }

            if (skinned)
            {
                ReadSkin(model, submesh, vertex, boneIndices.Offset,
                         boneWeights.Present ? boneWeights.Offset : -1, bones, weights, i);
            }
        }

        var indices = new int[triangles * 3];

        for (int i = 0; i < triangles; i++)
        {
            int a = model.Index(submesh.StartIndex + i * 3);
            int b = model.Index(submesh.StartIndex + i * 3 + 1);
            int c = model.Index(submesh.StartIndex + i * 3 + 2);

            if (a >= count || b >= count || c >= count)
            {
                error = "submesh index points outside its vertex range";
                return false;
            }

            indices[i * 3] = a;
            indices[i * 3 + 1] = c;
            indices[i * 3 + 2] = b;
        }

        if (!normal.Present)
        {
            BuildNormals(vertices, indices, normals);
        }

        var surface = new Godot.Collections.Array();
        surface.Resize((int)Mesh.ArrayType.Max);
        surface[(int)Mesh.ArrayType.Vertex] = vertices;
        surface[(int)Mesh.ArrayType.Normal] = normals;
        surface[(int)Mesh.ArrayType.TexUV] = uvs;
        surface[(int)Mesh.ArrayType.TexUV2] = uvs2;
        surface[(int)Mesh.ArrayType.Index] = indices;

        if (tangent.Present)
        {
            surface[(int)Mesh.ArrayType.Tangent] = tangents;
        }

        if (skinned)
        {
            surface[(int)Mesh.ArrayType.Bones] = bones;
            surface[(int)Mesh.ArrayType.Weights] = weights;
        }

        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, surface);
        mesh.SurfaceSetName(mesh.GetSurfaceCount() - 1, SurfaceName(model, submesh));
        return true;
    }

    private static string SurfaceName(M3File model, M3Submesh submesh)
    {
        string name = "material" + submesh.MaterialIndex;

        if (submesh.MaterialIndex >= model.Materials.Length)
        {
            return name;
        }

        M3Material material = model.Materials[submesh.MaterialIndex];
        if (material.Layers.Length == 0)
        {
            return name;
        }

        int texture = material.Layers[0].TextureA;
        if (texture >= model.Textures.Length)
        {
            return name;
        }

        string path = model.Textures[texture].Path;
        if (path.Length == 0)
        {
            return name;
        }

        int slash = path.LastIndexOfAny(new[] { '\\', '/' });
        return name + "_" + (slash >= 0 ? path[(slash + 1)..] : path);
    }

    private static void ReadSkin(M3File model, in M3Submesh submesh, ReadOnlySpan<byte> vertex,
                                 int indexAt, int weightAt,
                                 int[] bones, float[] weights, int i)
    {
        float total = 0.0f;

        for (int k = 0; k < 4; k++)
        {
            float weight = weightAt < 0
                ? (k == 0 ? 1.0f : 0.0f)
                : vertex[weightAt + k] * M3File.WeightScale;

            bones[i * 4 + k] = model.ResolveBone(submesh, vertex[indexAt + k]);
            weights[i * 4 + k] = weight;
            total += weight;
        }

        if (total <= 0.0f)
        {
            weights[i * 4] = 1.0f;
            return;
        }

        for (int k = 0; k < 4; k++)
        {
            weights[i * 4 + k] /= total;
        }
    }

    private static void BuildNormals(Vector3[] vertices, int[] indices, Vector3[] normals)
    {
        for (int i = 0; i + 2 < indices.Length; i += 3)
        {
            int a = indices[i];
            int b = indices[i + 1];
            int c = indices[i + 2];

            Vector3 face = (vertices[b] - vertices[a]).Cross(vertices[c] - vertices[a]);

            normals[a] += face;
            normals[b] += face;
            normals[c] += face;
        }

        for (int i = 0; i < normals.Length; i++)
        {
            normals[i] = normals[i].LengthSquared() > 0.0f ? normals[i].Normalized() : Vector3.Up;
        }
    }

    private static Vector3 DecodeOct(ReadOnlySpan<byte> vertex, int at)
    {
        float u = vertex[at] * M3File.WeightScale * 2.0f - 1.0f;
        float v = vertex[at + 1] * M3File.WeightScale * 2.0f - 1.0f;
        float z = 1.0f - (Math.Abs(u) + Math.Abs(v));

        if (z < 0.0f)
        {
            float su = u >= 0.0f ? 1.0f : -1.0f;
            float sv = v >= 0.0f ? 1.0f : -1.0f;
            float folded = su * (1.0f - Math.Abs(v));
            v = sv * (1.0f - Math.Abs(u));
            u = folded;
        }

        var decoded = new Vector3(u, v, z);
        return decoded.LengthSquared() > 0.0f ? decoded.Normalized() : Vector3.Up;
    }

    private static Vector3 ReadPosition(ReadOnlySpan<byte> vertex, M3VertexField field)
    {
        int at = field.Offset;

        if (field.Type == M3FieldType.Float3)
        {
            return new Vector3(BitConverter.ToSingle(vertex[at..]),
                               BitConverter.ToSingle(vertex[(at + 4)..]),
                               BitConverter.ToSingle(vertex[(at + 8)..]));
        }

        return new Vector3(Fixed(vertex, at), Fixed(vertex, at + 2), Fixed(vertex, at + 4));
    }

    private static Vector2 ReadHalf2(ReadOnlySpan<byte> vertex, int at) =>
        new(Half(vertex, at), Half(vertex, at + 2));

    private static float Fixed(ReadOnlySpan<byte> vertex, int at) =>
        BinaryPrimitives.ReadInt16LittleEndian(vertex[at..]) * M3File.PositionScale;

    private static float Half(ReadOnlySpan<byte> vertex, int at) =>
        HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(vertex[at..]));
}
