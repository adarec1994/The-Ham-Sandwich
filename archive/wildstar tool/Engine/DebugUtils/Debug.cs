using OpenTK;
using OpenTK.Graphics.OpenGL4;
using MathUtils;
using ProjectWS.Engine.Rendering;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Text;
using static System.Net.Mime.MediaTypeNames;

namespace ProjectWS
{
    public class Debug
    {
        const ConsoleColor LOG_COLOR = ConsoleColor.White;
        const ConsoleColor WARNING_COLOR = ConsoleColor.Yellow;
        const ConsoleColor ERROR_COLOR = ConsoleColor.DarkRed;
        const ConsoleColor EXCEPTION_COLOR = ConsoleColor.Red;

        public static TextRenderer? textRenderer;
        public static IconRenderer? iconRenderer;
        public static ImmediateRenderer? immRenderer;

        public static void Log(string text, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(text);
            Console.ResetColor();
        }

        public static void Log(object anything, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(anything.ToString());
            Console.ResetColor();
        }

        public static void Log(bool value, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(value.ToString());
            Console.ResetColor();
        }

        public static void Log(int value, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(value);
            Console.ResetColor();
        }

        public static void Log(uint value, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(value);
            Console.ResetColor();
        }

        public static void Log(float value, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(value);
            Console.ResetColor();
        }

        public static void Log(Matrix4 value, ConsoleColor color = LOG_COLOR)
        {
            Console.ForegroundColor = color;
            Console.WriteLine("Matrix4 " + value.ToString());
            Console.ResetColor();
        }

        public static void Log(string format, object arg0)
        {
            Console.ForegroundColor = LOG_COLOR;
            Console.WriteLine(format, arg0);
            Console.ResetColor();
        }

        public static void LogWarning(string text)
        {
            Console.ForegroundColor = WARNING_COLOR;
            Console.WriteLine(text);
            Console.ResetColor();
        }

        public static void LogError(string text)
        {
            Console.ForegroundColor = ERROR_COLOR;
            Console.WriteLine(text);
            Console.ResetColor();
        }

        public static void LogException(Exception e)
        {
            Console.ForegroundColor = EXCEPTION_COLOR;
            Console.WriteLine(e.Message);
            Console.WriteLine(e.StackTrace);
            Console.ResetColor();
        }

        public static void Log()
        {
            Console.WriteLine();
        }

        /// <summary>
        /// Renderd a label in 3D space using immediate mode
        /// </summary>
        /// <param name="text">The label text</param>
        /// <param name="position">World space position</param>
        /// <param name="color">Text color</param>
        /// <param name="shadow">Enable text shadow effect</param>
        public static void DrawLabel3D(string text, Vector3 position,  Vector4 color, bool shadow)
        {
            textRenderer?.DrawLabel3D(text, position, color, shadow);
        }

        public static void DrawIcon3D(IconRenderer.Icon3D.Type type, Vector3 position, Vector4 color)
        {
            iconRenderer?.DrawIcon3D(type, position, color);
        }
   
        /// <summary>
        /// Render a wireframe box using immediate mode
        /// </summary>
        /// <param name="position">World space position</param>
        /// <param name="rotation">Rotation</param>
        /// <param name="size">Size</param>
        public static void DrawWireBox3D(Vector3 position, Quaternion rotation, Vector3 size, Color color)
        {
            immRenderer?.DrawWireBox3D(position, rotation, size, color);
        }

        public static void DrawWireBox3D(Matrix4 matrix, Color color)
        {
            immRenderer?.DrawWireBox3D(matrix, color);
        }
    }
}
