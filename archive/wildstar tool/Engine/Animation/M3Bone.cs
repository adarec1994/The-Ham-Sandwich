using MathUtils;
using ProjectWS.Engine.Objects;
using ProjectWS.Engine.Objects.Gizmos;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Animation
{
    public class M3Bone
    {
        FileFormats.M3.Bone data;
        BoneGizmo boneGizmo;

        public M3Bone(FileFormats.M3.Bone data) 
        {
            this.data = data;
            this.boneGizmo = new Objects.Gizmos.BoneGizmo();
        }

        public void Build(Rendering.Renderer renderer, Engine engine, M3Model model)
        {
            renderer?.gizmos?.Add(this.boneGizmo);
            engine?.taskManager?.buildTasks.Enqueue(new TaskManager.BuildObjectTask(this.boneGizmo));

            this.boneGizmo.transform.SetPosition(this.data.pivot);

            if (this.data.parentId != -1)
            {
                Vector3 eye = this.data.pivot;
                Vector3 target = model.bones[this.data.parentId].data.pivot;

                Matrix4 view = Matrix4.LookAt(eye, target, new Vector3(0.0f, 0.0f, 1.0f));
                view.Transpose();
                this.boneGizmo.transform.SetRotation(view.ExtractRotation());
                this.boneGizmo.transform.SetScale(Vector3.Distance(eye, target) * Vector3.One);
                //this.boneGizmo.transform.SetMatrix(view);
            }

            //this.boneGizmo.transform.SetRotation(this.data.bindPose.ExtractRotation());
        }
    }
}
