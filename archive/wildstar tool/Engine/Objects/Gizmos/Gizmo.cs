using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Objects.Gizmos
{
    public class Gizmo : GameObject
    {
        public LineVertexBuffer[] GenerateCircleVertices(float radius, int numSegments, float height, Vector3 axis, Vector4 color)
        {
            LineVertexBuffer[] vertices = new LineVertexBuffer[numSegments + 1];
            float angleIncrement = (float)(2 * Math.PI / numSegments);

            for (int i = 0; i <= numSegments; i++)
            {
                float angle = i * angleIncrement;
                float x = radius * (float)Math.Cos(angle);
                float y = radius * (float)Math.Sin(angle);
                if (axis == Vector3.UnitX)
                    vertices[i] = new LineVertexBuffer(new Vector3(height, x, y), color);
                else if (axis == Vector3.UnitY)
                    vertices[i] = new LineVertexBuffer(new Vector3(x, height, y), color);
                else if (axis == Vector3.UnitZ)
                    vertices[i] = new LineVertexBuffer(new Vector3(x, y, height), color);
            }

            return vertices;
        }

        public static LineVertexBuffer[] GenerateWireSphereVertices(float radius, int numSegments, Vector4 color)
        {
            LineVertexBuffer[] vertices = new LineVertexBuffer[numSegments * 3];

            for (int i = 0; i < numSegments; i++)
            {
                float angle = (float)(2 * Math.PI * i / numSegments);

                // X-axis circle
                vertices[i] = new LineVertexBuffer(new Vector3(radius * (float)Math.Cos(angle), radius * (float)Math.Sin(angle), 0), color);

                // Y-axis circle
                vertices[i + numSegments] = new LineVertexBuffer(new Vector3(0, radius * (float)Math.Cos(angle), radius * (float)Math.Sin(angle)), color);

                // Z-axis circle
                vertices[i + numSegments * 2] = new LineVertexBuffer(new Vector3(radius * (float)Math.Cos(angle), 0, radius * (float)Math.Sin(angle)), color);
            }

            return vertices;
        }

        public LineVertexBuffer[] GenerateSphereVertices(float radius, int numSegments, Vector4 color)
        {
            LineVertexBuffer[] vertices = new LineVertexBuffer[(numSegments + 1) * (numSegments + 1)];
            float phiIncrement = (float)(Math.PI / numSegments);
            float thetaIncrement = (float)(2 * Math.PI / numSegments);
            int vertexIndex = 0;

            for (int phiIndex = 0; phiIndex <= numSegments; phiIndex++)
            {
                float phi = phiIndex * phiIncrement;

                for (int thetaIndex = 0; thetaIndex <= numSegments; thetaIndex++)
                {
                    float theta = thetaIndex * thetaIncrement;

                    float x = radius * (float)(Math.Sin(phi) * Math.Cos(theta));
                    float y = radius * (float)(Math.Sin(phi) * Math.Sin(theta));
                    float z = radius * (float)Math.Cos(phi);

                    vertices[vertexIndex++] = new LineVertexBuffer(new Vector3(x, y, z), color);
                }
            }

            return vertices;
        }

        public int[] GenerateSphereIndices(int numSegments)
        {
            int[] indices = new int[numSegments * numSegments * 6];
            int index = 0;

            for (int phiIndex = 0; phiIndex < numSegments; phiIndex++)
            {
                for (int thetaIndex = 0; thetaIndex < numSegments; thetaIndex++)
                {
                    int currentRow = phiIndex * (numSegments + 1);
                    int nextRow = (phiIndex + 1) * (numSegments + 1);

                    int vertexIndex = currentRow + thetaIndex;
                    indices[index++] = vertexIndex;
                    indices[index++] = vertexIndex + 1;
                    indices[index++] = nextRow + thetaIndex;

                    indices[index++] = vertexIndex + 1;
                    indices[index++] = nextRow + thetaIndex + 1;
                    indices[index++] = nextRow + thetaIndex;
                }
            }

            return indices;
        }

        public int[] GenerateCircleIndices(int numSegments)
        {
            int[] indices = new int[numSegments * 2 + 2];
            int index = 0;

            for (int i = 0; i < numSegments; i++)
            {
                indices[index++] = i;
                indices[index++] = i + 1;
            }

            // Closing the loop
            indices[index++] = numSegments;
            indices[index] = 0;

            return indices;
        }

        public static int[] GenerateWireSphereIndices(int numSegments)
        {
            int[] indices = new int[numSegments * 3 * 2];
            int index = 0;

            for (int i = 0; i < numSegments; i++)
            {
                // X-axis circle indices
                indices[index++] = i;
                indices[index++] = (i + 1) % numSegments;

                // Y-axis circle indices
                indices[index++] = i + numSegments;
                indices[index++] = ((i + 1) % numSegments) + numSegments;

                // Z-axis circle indices
                indices[index++] = i + numSegments * 2;
                indices[index++] = ((i + 1) % numSegments) + numSegments * 2;
            }

            return indices;
        }
    }
}
