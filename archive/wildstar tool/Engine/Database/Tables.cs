using System.Collections;
using System.Collections.Generic;

namespace ProjectWS.Engine.Database
{
    public class Tables
    {
        public Data.GameData data;
        public Tbl<Definitions.MapContinent> mapContinent;
        public Tbl<Definitions.WorldLocation2> worldLocation;
        public Tbl<Definitions.World> world;
        public Tbl<Definitions.WorldLayer> worldLayer;
        public Tbl<Definitions.ModelBone> modelBone;
        public Tbl<Definitions.Creature2> creature2;
        public Tbl<Definitions.Creature2DisplayInfo> creature2DisplayInfo;
        public Tbl<Definitions.WorldSky> worldSky;

        public Tables(Engine engine, Data.GameData data)
        {
            this.data = data;
            engine.taskManager.otherThread.Enqueue(new TaskManager.DatabaseTask(this, TaskManager.Task.JobType.Read, engine.taskManager));
        }

        public void Read()
        {
            this.mapContinent = Tbl<Definitions.MapContinent>.Open($"{this.data.gamePath}\\Data\\DB\\MapContinent.tbl");
            this.worldLocation = Tbl<Definitions.WorldLocation2>.Open($"{this.data.gamePath}\\Data\\DB\\WorldLocation2.tbl");
            this.world = Tbl<Definitions.World>.Open($"{this.data.gamePath}\\Data\\DB\\World.tbl");
            this.worldLayer = Tbl<Definitions.WorldLayer>.Open($"{this.data.gamePath}\\Data\\DB\\WorldLayer.tbl");
            this.modelBone = Tbl<Definitions.ModelBone>.Open($"{this.data.gamePath}\\Data\\DB\\ModelBone.tbl");
            this.creature2 = Tbl<Definitions.Creature2>.Open($"{this.data.gamePath}\\Data\\DB\\Creature2.tbl");
            this.creature2DisplayInfo = Tbl<Definitions.Creature2DisplayInfo>.Open($"{this.data.gamePath}\\Data\\DB\\Creature2DisplayInfo.tbl");
            this.worldSky = Tbl<Definitions.WorldSky>.Open($"{this.data.gamePath}\\Data\\DB\\WorldSky.tbl");
            this.data.databaseAvailable = true;
            /*
            this.mapContinent = Tbl<Definitions.MapContinent>.Open(this.data);
            this.worldLocation = Tbl<Definitions.WorldLocation2>.Open(this.data);
            this.world = Tbl<Definitions.World>.Open(this.data);
            this.worldLayer = Tbl<Definitions.WorldLayer>.Open(this.data);
            this.modelBone = Tbl<Definitions.ModelBone>.Open(this.data);
            this.creature2 = Tbl<Definitions.Creature2>.Open(this.data);
            this.creature2DisplayInfo = Tbl<Definitions.Creature2DisplayInfo>.Open(this.data);
            this.worldSky = Tbl<Definitions.WorldSky>.Open(this.data);
            this.data.databaseAvailable = true;
            */
        }
    }
}