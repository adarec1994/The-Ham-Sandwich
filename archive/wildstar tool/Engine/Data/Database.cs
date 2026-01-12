using ProjectWS.Engine.Database;

namespace ProjectWS.Engine.Data
{
    public class Database
    {
        public Tbl<ProjectWS.Engine.Database.Definitions.MapContinent>? mapContinent;
        public Tbl<ProjectWS.Engine.Database.Definitions.WorldLocation2>? worldLocation;
        public Tbl<ProjectWS.Engine.Database.Definitions.World>? world;
        public Tbl<ProjectWS.Engine.Database.Definitions.WorldLayer>? worldLayer;
        public Tbl<ProjectWS.Engine.Database.Definitions.ModelBone>? modelBone;
        public Tbl<ProjectWS.Engine.Database.Definitions.Creature2>? creature2;
        public Tbl<ProjectWS.Engine.Database.Definitions.Creature2DisplayInfo>? creature2DisplayInfo;
        public Tbl<ProjectWS.Engine.Database.Definitions.WorldSky>? worldSky;

        public Database(Engine engine)
        {
            Read();
            //engine.taskManager.otherThread.Enqueue(new TaskManager.DatabaseTask(this, TaskManager.Task.JobType.Read, engine.taskManager));
        }

        public void Read()
        {
            var assetDBPath = Path.GetDirectoryName(Engine.settings.dataManager.assetDatabasePath);
            this.mapContinent = Tbl<ProjectWS.Engine.Database.Definitions.MapContinent>.Open($"{assetDBPath}\\DB\\MapContinent.tbl");
            this.worldLocation = Tbl<ProjectWS.Engine.Database.Definitions.WorldLocation2>.Open($"{assetDBPath}\\DB\\WorldLocation2.tbl");
            this.world = Tbl<ProjectWS.Engine.Database.Definitions.World>.Open($"{assetDBPath}\\DB\\World.tbl");
            this.worldLayer = Tbl<ProjectWS.Engine.Database.Definitions.WorldLayer>.Open($"{assetDBPath}\\DB\\WorldLayer.tbl");
            this.modelBone = Tbl<ProjectWS.Engine.Database.Definitions.ModelBone>.Open($"{assetDBPath}\\DB\\ModelBone.tbl");
            this.creature2 = Tbl<ProjectWS.Engine.Database.Definitions.Creature2>.Open($"{assetDBPath}\\DB\\Creature2.tbl");
            this.creature2DisplayInfo = Tbl<ProjectWS.Engine.Database.Definitions.Creature2DisplayInfo>.Open($"{assetDBPath}\\DB\\Creature2DisplayInfo.tbl");
            this.worldSky = Tbl<ProjectWS.Engine.Database.Definitions.WorldSky>.Open($"{assetDBPath}\\DB\\WorldSky.tbl");
        }
    }
}

