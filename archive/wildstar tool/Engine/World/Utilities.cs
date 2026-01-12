using OpenTK;
using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.World
{
    public static class Utilities
    {
        static Random random;

        public static Vector2i WorldToChunkCoords(Vector3 worldCoords)
        {
            int X = FloorToInt(64 + (worldCoords.X / World.AREA_SIZE));
            int Y = FloorToInt(64 + (worldCoords.Z / World.AREA_SIZE));
            return new Vector2i(X, Y);
        }

        public static Vector3 ChunkToWorldCoords(Vector2 chunkCoords)
        {
            return new Vector3((chunkCoords.X - 64) * World.AREA_SIZE, 0, (chunkCoords.Y - 64) * World.AREA_SIZE);
        }

        public static Vector2 CalculateLowCoords(Vector2 chunkCoords)
        {
            return new Vector2(FloorToInt(chunkCoords.X / 8f), FloorToInt(chunkCoords.Y / 8f));
        }

        public static int[] CalculateLoDQuadrants(Vector2 chunkCoords, Vector2 lowCoords)
        {
            // TODO : this shit ain't working

            int x = (int)(chunkCoords.X - (lowCoords.X * 8));
            int y = (int)(chunkCoords.Y - (lowCoords.Y * 8));
            int[] quadrants = new int[4];

            quadrants[0] = x + (y * 16);
            quadrants[1] = x + (y * 16) + 1;
            quadrants[2] = x + (y * 16) + 16;
            quadrants[3] = x + (y * 16) + 16 + 1;
            /*
            quadrants[0] = (x * 16) - (16 - y);
            quadrants[1] = ((x + 1) * 16) - (16 - y);
            quadrants[2] = (x * 16) - (16 - (y + 1));
            quadrants[3] = ((x + 1) * 16) - (16 - (y + 1));
            */
            return quadrants;
        }

        public static int FloorToInt(float val)
        {
            return (int)Math.Floor(val);
        }

        public static int Random()
        {
            if (random == null)
                random = new Random();

            return random.Next();
        }

        public static int RandomRange(int min, int max)
        {
            if (random == null)
                random = new Random();

            return random.Next(min, max);
        }
    }
}
