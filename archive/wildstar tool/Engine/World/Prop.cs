using MathUtils;
using ProjectWS.Engine.Data;
using ProjectWS.Engine.Material;
using ProjectWS.Engine.Rendering;
using System;
using System.Collections.Concurrent;

namespace ProjectWS.Engine.World
{
    public class Prop
    {
        //public FileFormats.Area.AreaProp? areaprop;

        // Instance Buffers //
        public ConcurrentDictionary<uint, Instance>? instances;
        public List<Matrix4>? renderableInstances;
        public HashSet<uint>? renderableUUIDs;
        public Dictionary<uint, bool>? cullingResults;                // This indicates if instance is culled or not
        public List<uint>? uniqueInstanceIDs;

        public FileFormats.M3.File? data;
        public M3Geometry[]? geometries;
        public M3Material[]? materials;
        public AABB? aabb;
        public bool culled;                             // Determined if renderableInstances.Count == 0
        private bool isBuilt;
        public bool hasMeshes;
        Engine? engine;

        private void Setup(FileFormats.M3.File data)
        {
            if (data.bounds != null)
            {
                this.aabb = data.bounds[0].bbA;
            }
            else
            {
                this.aabb = new AABB(Vector3.Zero, Vector3.One);
            }

            this.cullingResults = new Dictionary<uint, bool>();
            this.renderableInstances = new List<Matrix4>();
            this.renderableUUIDs = new HashSet<uint>();
            this.instances = new ConcurrentDictionary<uint, Instance>();
            this.uniqueInstanceIDs = new List<uint>();
        }

        public Prop(FileFormats.M3.File data, FileFormats.Area.AreaProp areaprop, Engine engine)
        {
            this.data = data;
            //this.areaprop = areaprop;
            this.engine = engine;
            Setup(data);

            if (this.data == null) return;
            if (this.data.geometries == null) return;
            if (this.data.geometries.Length <= 0) return;
            if (this.data.geometries[0].submeshes == null) return;

            AddInstance(areaprop);
            Build();
        }

        public void Build()
        {
            this.materials = new M3Material[this.data.materials.Length];
            for (int i = 0; i < this.materials.Length; i++)
            {
                this.materials[i] = new M3Material(this.data.materials[i], this.data, this.engine.resourceManager);
                this.materials[i].Build();
            }

            this.geometries = new M3Geometry[this.data.geometries.Length];

            for (int i = 0; i < this.geometries.Length; i++)
            {
                this.geometries[i] = new M3Geometry(this.data.geometries[i]);
                this.geometries[i].Build(this.data.modelID);

                if (this.geometries[i].meshes?.Length > 0)
                    this.hasMeshes = true;
            }

            this.isBuilt = true;
        }

        internal void AddInstance(FileFormats.Area.AreaProp areaprop)
        {
            //this.areaprop = areaprop;
            //AddInstance(areaprop.uniqueID, areaprop.position, areaprop.rotation, areaprop.scale * Vector3.One);

            var instance = new Instance(areaprop);
            instance.obb = new OBB(this.aabb, areaprop.rotation);
            instance.aabb = instance.obb.GetEncapsulatingAABB();

            this.instances?.TryAdd(areaprop.uniqueID, instance);
            this.uniqueInstanceIDs?.Add(areaprop.uniqueID);
            this.cullingResults[areaprop.uniqueID] = false;
        }

        public void Render(Shader shader, Matrix4 model)
        {
            if (!this.isBuilt) return;

            for (int g = 0; g < this.geometries?.Length; g++)
            {
                for (int i = 0; i < this.geometries[g].meshes?.Length; i++)
                {
                    var mesh = this.geometries[g].meshes[i];

                    if (mesh == null || !mesh.isBuilt || mesh.data == null) continue;

                    int matSelector = mesh.data.materialSelector;

                    var mat = model;

                    if (mesh.positionCompressed)
                        mat = WorldRenderer.decompressMat * model;

                    shader.SetMat4("model", ref mat);

                    this.materials?[matSelector].SetToShader(shader);

                    mesh.Draw();
                    Rendering.WorldRenderer.propDrawCalls++;
                }
            }
        }

        internal void Unload()
        {
            this.instances?.Clear();
            this.renderableInstances?.Clear();
            this.renderableUUIDs?.Clear();
            this.cullingResults?.Clear();
            this.uniqueInstanceIDs?.Clear();
            this.data = null;

            for (int g = 0; g < this.geometries?.Length; g++)
            {
                for (int m = 0; m < this.geometries[g]?.meshes?.Length; m++)
                {
                    this.geometries[g]?.meshes?[m]?.Unload();
                }
            }

            this.geometries = null;

            for (int m = 0; m < this.materials?.Length; m++)
            {
                this.materials[m].Unload();
            }

            this.materials = null;
        }

        public class Instance
        {
            public FileFormats.Area.AreaProp? areaprop;
            public Matrix4 transform;
            public Vector3 position;
            public Quaternion rotation;
            public Vector3 rotationEuler;
            public Vector3 scale;
            public Type type;
            public uint uuid;
            internal bool visible;
            public OBB? obb;
            public AABB? aabb;

            //public Rect screenSpace;

            public Instance(Matrix4 transform, uint uuid)
            {
                this.transform = transform;
                this.uuid = uuid;
                this.type = Type.WorldProp;
                
                // TODO : Fix TRS extracting and assign pos/rot/scale
            }

            public Instance(Vector3 position, Quaternion rotation, Vector3 scale, uint uuid)
            {
                Matrix4 mat = Matrix4.Identity;
                this.transform = mat.TRS(position, rotation, scale);

                this.uuid = uuid;
                this.type = Type.WorldProp;
                this.position = position;
                this.rotation = rotation;
                this.scale = scale;
                rotation.ToEulerAngles(out this.rotationEuler);
                // Convert to Degree
                this.rotationEuler *= (float)(180f / Math.PI);
            }

            public Instance(FileFormats.Area.AreaProp areaprop)
            {
                this.areaprop = areaprop;

                Matrix4 mat = Matrix4.Identity;
                this.transform = mat.TRS(areaprop.position, areaprop.rotation, areaprop.scale * Vector3.One);

                this.uuid = areaprop.uniqueID;
                this.type = Type.WorldProp;
                this.position = areaprop.position;
                this.rotation = areaprop.rotation;
                this.scale = areaprop.scale * Vector3.One;
                areaprop.rotation.ToEulerAngles(out this.rotationEuler);
                // Convert to Degree
                this.rotationEuler *= (float)(180f / Math.PI);
            }

            public enum Type
            {
                WorldProp,
                I3Prop,
            }
        }
    }
}
