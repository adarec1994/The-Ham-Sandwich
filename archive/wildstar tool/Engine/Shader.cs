using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL4;
using MathUtils;
using System;
using System.IO;
using System.Text;
using System.Xml.Linq;

namespace ProjectWS.Engine
{
    public class Shader
    {
        public int Handle;
        string vertexPath;
        string fragmentPath;

        public Shader(string vertexPath, string fragmentPath)
        {
            this.vertexPath = vertexPath;
            this.fragmentPath = fragmentPath;
            Load();
        }

        public void Load()
        {
            int VertexShader;
            int FragmentShader;

            string VertexShaderSource;

            try
            {
                using (StreamReader reader = new StreamReader(vertexPath, Encoding.UTF8))
                {
                    VertexShaderSource = reader.ReadToEnd();
                }
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
                return;
            }
            string FragmentShaderSource;

            try
            {
                using (StreamReader reader = new StreamReader(fragmentPath, Encoding.UTF8))
                {
                    FragmentShaderSource = reader.ReadToEnd();
                }
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
                return;
            }

            VertexShader = GL.CreateShader(ShaderType.VertexShader);
            GL.ShaderSource(VertexShader, VertexShaderSource);

            FragmentShader = GL.CreateShader(ShaderType.FragmentShader);
            GL.ShaderSource(FragmentShader, FragmentShaderSource);

            GL.CompileShader(VertexShader);

            string infoLogVert = GL.GetShaderInfoLog(VertexShader);
            if (infoLogVert != System.String.Empty)
                Console.WriteLine(infoLogVert);

            GL.CompileShader(FragmentShader);

            string infoLogFrag = GL.GetShaderInfoLog(FragmentShader);

            if (infoLogFrag != System.String.Empty)
                Console.WriteLine(infoLogFrag);

            Handle = GL.CreateProgram();

            GL.AttachShader(Handle, VertexShader);
            GL.AttachShader(Handle, FragmentShader);

            GL.LinkProgram(Handle);

            GL.DetachShader(Handle, VertexShader);
            GL.DetachShader(Handle, FragmentShader);
            GL.DeleteShader(FragmentShader);
            GL.DeleteShader(VertexShader);
        }

        public void SetColor(string name, Color value)
        {
            GL.Uniform4(GL.GetUniformLocation(Handle, name), value.R, value.G, value.B, value.A);
        }

        public void SetColor4(string name, Vector4 value)
        {
            GL.Uniform4(GL.GetUniformLocation(Handle, name), value.X, value.Y, value.Z, value.W);
        }

        public void Use()
        {
            GL.UseProgram(Handle);
        }

        public void SetInt(string name, int value)
        {
            GL.Uniform1(GL.GetUniformLocation(Handle, name), value);
        }

        public void SetFloat(string name, float value)
        {
            GL.Uniform1(GL.GetUniformLocation(Handle, name), value);
        }

        public void SetMat4(string name, ref Matrix4 mat)
        {
            //GL.UniformMatrix4(GL.GetUniformLocation(Handle, name), false, ref mat);

            unsafe
            {
                fixed (float* value = &mat.Row0.X)
                {
                    GL.UniformMatrix4(GL.GetUniformLocation(Handle, name), 1, false, value);
                }
            }
        }

        public void SetVec3(string name, Vector3 vec)
        {
            GL.Uniform3(GL.GetUniformLocation(Handle, name), vec.X, vec.Y, vec.Z);
        }

        public void SetVec3(string name, float x, float y, float z)
        {
            GL.Uniform3(GL.GetUniformLocation(Handle, name), x, y, z);
        }

        public void SetVec4(string name, Vector4 vec)
        {
            GL.Uniform4(GL.GetUniformLocation(Handle, name), vec.X, vec.Y, vec.Z, vec.W);
        }

        public void SetTexture(string name)
        {
            var location = GL.GetUniformLocation(Handle, name);
            GL.Uniform1(location, 0);
        }

        private bool disposedValue = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                GL.DeleteProgram(Handle);

                disposedValue = true;
            }
        }

        ~Shader()
        {
            //GL.DeleteProgram(Handle);
        }


        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal void SetVec2(string name, float x, float y)
        {
            GL.Uniform2(GL.GetUniformLocation(Handle, name), x, y);
        }

        internal void SetVec2(string name, Vector2 vec)
        {
            GL.Uniform2(GL.GetUniformLocation(Handle, name), vec.X, vec.Y);
        }
    }
}