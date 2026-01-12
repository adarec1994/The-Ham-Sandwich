using MathUtils;
using OpenTK.Graphics.OpenGL4;
//using OpenTK.Mathematics;
using SharpFont;
using System.Reflection;
using static FontGenerator.FreeType;

namespace FontGenerator
{
    public class FreeType
    {
        Dictionary<uint, Character> characters;
        int _vbo;
        int _vao;
        bool init = false;

        public struct Character
        {
            public int TextureID { get; set; }
            public Vector2 Size { get; set; }
            public Vector2 Bearing { get; set; }
            public int Advance { get; set; }
        }

        public void Init()
        {
            //if (init) return;

            this.characters = new Dictionary<uint, Character>();

            // initialize library
            Library lib = new Library();
            Face face = new Face(lib, "Fonts/Arial.ttf");
            face.SetPixelSizes(0, 32);

            // set 1 byte pixel alignment 
            GL.PixelStore(PixelStoreParameter.UnpackAlignment, 1);

            // Load first 128 characters of ASCII set
            for (uint c = 0; c < 128; c++)
            {
                try
                {
                    // load glyph
                    face.LoadChar(c, LoadFlags.Render, LoadTarget.Normal);
                    GlyphSlot glyph = face.Glyph;
                    FTBitmap bitmap = glyph.Bitmap;

                    // create glyph texture
                    int texObj = GL.GenTexture();
                    GL.BindTexture(TextureTarget.Texture2D, texObj);
                    GL.TexImage2D(TextureTarget.Texture2D, 0,
                                  PixelInternalFormat.R8, bitmap.Width, bitmap.Rows, 0,
                                  PixelFormat.Red, PixelType.UnsignedByte, bitmap.Buffer);

                    // set texture parameters
                    GL.TextureParameter(texObj, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
                    GL.TextureParameter(texObj, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
                    GL.TextureParameter(texObj, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
                    GL.TextureParameter(texObj, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);

                    // add character
                    Character ch = new Character();
                    ch.TextureID = texObj;
                    ch.Size = new Vector2(bitmap.Width, bitmap.Rows);
                    ch.Bearing = new Vector2(glyph.BitmapLeft, glyph.BitmapTop);
                    ch.Advance = (int)glyph.Advance.X.Value;
                    characters.Add(c, ch);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                }
            }

            // bind default texture
            GL.BindTexture(TextureTarget.Texture2D, 0);

            // set default (4 byte) pixel alignment 
            GL.PixelStore(PixelStoreParameter.UnpackAlignment, 4);

            float[] vquad =
            {
                // x      y      u     v    
                    0.0f, -1.0f,   0.0f, 0.0f,
                    0.0f,  0.0f,   0.0f, 1.0f,
                    1.0f,  0.0f,   1.0f, 1.0f,
                    0.0f, -1.0f,   0.0f, 0.0f,
                    1.0f,  0.0f,   1.0f, 1.0f,
                    1.0f, -1.0f,   1.0f, 0.0f
                };

            // Create [Vertex Buffer Object](https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_Buffer_Object)
            _vbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, 4 * 6 * 4, vquad, BufferUsageHint.StaticDraw);

            // [Vertex Array Object](https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_Array_Object)
            _vao = GL.GenVertexArray();
            GL.BindVertexArray(_vao);
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 2, VertexAttribPointerType.Float, false, 4 * 4, 0);
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, 4 * 4, 2 * 4);

            init = true;
        }

        public void RenderText(string text, float x, float y, float scale, Vector2 dir, bool centered)
        {
            if (!init) return;

            GL.ActiveTexture(TextureUnit.Texture0);
            GL.BindVertexArray(_vao);

            float angle_rad = (float)Math.Atan2(dir.Y, dir.X);
            Matrix4 rotateM = Matrix4.CreateRotationZ(angle_rad);
            Matrix4 transOriginM = Matrix4.CreateTranslation(new Vector3(x, y, 0f));

            Character ch = characters['A'];
            Character templateChar = characters['A'];

            // Calculate size
            float textLength = 0;
            if (centered)
            {
                for (int i = 0; i < text.Length; i++)
                {
                    var c = text[i];

                    if (c == '\n')
                    {
                        // New row
                        textLength = 0;
                        continue;
                    }

                    if (characters.ContainsKey(c) == false)
                        continue;
                    ch = characters[c];

                    textLength += (ch.Size.X + ch.Bearing.X) * scale;
                }
            }

            // Iterate through all characters and render
            float char_x = 0.0f;
            float char_y = 0.0f;

            for (int i = 0; i < text.Length; i++)
            {
                var c = text[i];

                if (c == '\n')
                {
                    // New row
                    char_x = 0;
                    char_y += (templateChar.Size.Y) * scale * 1.5f;
                    continue;
                }

                if (characters.ContainsKey(c) == false)
                    continue;
                ch = characters[c];

                float w = ch.Size.X * scale;
                float h = ch.Size.Y * scale;//-
                float xrel = char_x + ch.Bearing.X * scale;
                float yrel = char_y + (ch.Size.Y - ch.Bearing.Y) * scale;//-

                if (centered)
                    xrel -= textLength / 2.0f;

                // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                char_x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))

                Matrix4 scaleM = Matrix4.CreateScale(new Vector3(w, h, 1.0f));
                Matrix4 transRelM = Matrix4.CreateTranslation(new Vector3(xrel, yrel, 0.0f));

                Matrix4 modelM = scaleM * transRelM * rotateM * transOriginM; // OpenTK `*`-operator is reversed
                //GL.UniformMatrix4(0, false, ref modelM);
                SetMat(ref modelM);

                // Render glyph texture over quad
                GL.BindTexture(TextureTarget.Texture2D, ch.TextureID);

                // Render quad
                GL.DrawArrays(PrimitiveType.Triangles, 0, 6);
            }

            GL.BindVertexArray(0);
            GL.BindTexture(TextureTarget.Texture2D, 0);
        }

        unsafe void SetMat(ref Matrix4 mat)
        {
            fixed (float* value = &mat.Row0.X)
            {
                GL.UniformMatrix4(0, 1, false, value);
            }
        }
    }
}
