using System.Collections;
using System.Collections.Generic;
using OpenTK;
using MathUtils;
using ProjectWS.FileFormats.Area;
using System;

namespace ProjectWS.Engine.Data.ResourceManager
{
    public class ModelResource
    {
        // Identifiers //
        string filePath;

        // Reference //
        World.World world;

        // Data //
        public FileFormats.M3.File m3;
        public Manager.ResourceState state;

        // Info //
        int referenceCount;

        // Usage //
        public List<ModelReference> modelReferences;

        public ModelResource(string filePath, World.World world)
        {
            this.world = world;
            this.filePath = filePath;
            this.state = Manager.ResourceState.IsLoading;
            this.modelReferences = new List<ModelReference>();
            this.m3 = new FileFormats.M3.File(filePath);
        }

        public void SetFileState(Manager.ResourceState state)
        {
            this.state = state;
        }

        public void BuildAllRefs()
        {
            for (int i = 0; i < modelReferences.Count; i++)
            {
                var item = this.modelReferences[i];
                //if (item.areaprop != null)
                    this.world.LoadProp(this.m3, item.areaprop);
                //else
                //    this.world.LoadProp(this.m3, item.uuid, item.position, item.rotation, item.scale);
            }
        }
        /*
        public void TryBuildObject(uint uuid, Vector3 position, Quaternion rotation, Vector3 scale)
        {
            if (this.state != Manager.ResourceState.IsReady)
            {
                // Unavailable //
                this.modelReferences.Add(new ModelReference(uuid, position, rotation, scale));
                referenceCount++;
            }
            else
            {
                this.world.LoadProp(this.m3, uuid, position, rotation, scale);
            }
        }
        */
        internal void TryBuildObject(AreaProp areaprop)
        {
            if (this.state != Manager.ResourceState.IsReady)
            {
                // Unavailable //
                this.modelReferences.Add(new ModelReference(areaprop));
                referenceCount++;
            }
            else
            {
                this.world.LoadProp(this.m3, areaprop);
            }
        }

        public struct ModelReference
        {
            public AreaProp? areaprop { get; set; }
            public Vector3 position { get; set; }
            public Quaternion rotation { get; set; }
            public Vector3 scale { get; set; }
            public uint uuid { get; set; }

            public ModelReference(uint uuid, Vector3 position, Quaternion rotation, Vector3 scale)
            {
                this.areaprop = null;
                this.uuid = uuid;
                this.position = position;
                this.rotation = rotation;
                this.scale = scale;
            }

            public ModelReference(AreaProp areaprop)
            {
                this.areaprop = areaprop;
                this.position = areaprop.position;
                this.rotation = areaprop.rotation;
                this.scale = areaprop.scale * Vector3.One;
                this.uuid = areaprop.uniqueID;
            }
        }
    }
}
