using ProjectWS.Engine.Data.DataProcess;
using System.ComponentModel;
using System.IO;
using System.Text.Json;

namespace ProjectWS.Engine.Data
{
    public static class DataManager
    {
        public static Engine? engine;
        public static AssetDatabase? assetDB;
        public static string? assetDatabasePath;
        public static string? assetDatabaseDir;
        public const string ASSETDB_FILENAME = "AssetDatabase.json";

        public const int ASSETDB_VERSION = 1;

        public static Thread? thread;
        public static Action<float>? logProgress;
        public static Action<string?>? logProgressText;

        public static bool assetDBReady;
        public static Database? database;


        public static void LoadAssetDatabase(string path)
        {
            if (File.Exists(path))
            {
                string? jsonString = File.ReadAllText(path);
                if (jsonString != null && jsonString != String.Empty)
                {
                    assetDB = JsonSerializer.Deserialize<AssetDatabase>(jsonString);
                    assetDatabasePath = path;
                    assetDatabaseDir = Path.GetDirectoryName(path);
                }
            }
            else
            {
                return;
            }

            if (assetDB.database != AssetDatabase.DataStatus.Ready ||
                assetDB.gameArt != AssetDatabase.DataStatus.Ready)
            {
                assetDBReady = false;
                Debug.LogWarning("Asset Database only partially created, go to Data Manager and click Build to continue the process.");
            }
            else
            {
                assetDBReady = true;
            }

            database = new Database(engine);
        }

        public static void CreateAssetDB(string path, Action<float>? _logProgress = null, Action<string?>? _logProgressText = null)
        {
            if (File.Exists(path))
            {
                string? jsonString = File.ReadAllText(path);
                if (jsonString != null && jsonString != String.Empty)
                {
                    assetDB = JsonSerializer.Deserialize<AssetDatabase>(jsonString);
                }
            }
            else
            {
                assetDB = new AssetDatabase();
                assetDB.version = DataManager.ASSETDB_VERSION;
            }

            logProgress = _logProgress;
            logProgressText = _logProgressText;

            SaveAssetDB(path);

            SharedProcessData spd = new SharedProcessData();
            spd.engineRef = engine;
            spd.gameClientPath = Engine.settings.dataManager?.gameClientPath;
            spd.assetDBFolder = Path.GetDirectoryName(Engine.settings.dataManager?.assetDatabasePath);
            spd.assetDB = assetDB;

            BackgroundWorker worker = new BackgroundWorker();
            spd.workerRef = worker;
            worker.WorkerReportsProgress = true;
            worker.ProgressChanged += new ProgressChangedEventHandler(
            (sender, e) =>
            {
                logProgress?.Invoke(e.ProgressPercentage);
                if (e.UserState != null)
                    logProgressText?.Invoke(e.UserState as String);
            });
            worker.DoWork +=
            (s3, e3) =>
            {
                new PLoadGameData(spd).Run();

                if (assetDB?.database != AssetDatabase.DataStatus.Ready)
                {
                    assetDB.database = AssetDatabase.DataStatus.InProgress;
                    SaveAssetDB(path);
                    new PExtractDatabase(spd).Run();
                    database = new Database(engine);
                    assetDB.database = AssetDatabase.DataStatus.Ready;
                    SaveAssetDB(path);
                }
                if (assetDB?.gameArt != AssetDatabase.DataStatus.Ready)
                {
                    assetDB.gameArt = AssetDatabase.DataStatus.InProgress;
                    SaveAssetDB(path);
                    new PExtractArt(spd).Run();
                    assetDB.gameArt = AssetDatabase.DataStatus.Ready;
                    SaveAssetDB(path);
                }

                new PEnd(spd).Run();
            };

            worker.RunWorkerAsync();
        }

        static void SaveAssetDB(string path)
        {
            if (path != null)
            {
                var dir = Path.GetDirectoryName(path);
                if (!Directory.Exists(dir))
                    Directory.CreateDirectory(dir);

                var options = new JsonSerializerOptions { WriteIndented = true };
                string data = JsonSerializer.Serialize(assetDB, options);
                File.WriteAllText(path, data);
            }
        }

        public static void ExportAllGameData(string selectedPath, Action<float>? _logProgress = null, Action<string?>? _logProgressText = null)
        {
            logProgress = _logProgress;
            logProgressText = _logProgressText;


            SharedProcessData spd = new SharedProcessData();
            spd.engineRef = engine;
            spd.gameClientPath = Engine.settings.dataManager?.gameClientPath;

            BackgroundWorker worker = new BackgroundWorker();
            spd.workerRef = worker;
            spd.extractPath = selectedPath;
            worker.WorkerReportsProgress = true;
            worker.ProgressChanged += new ProgressChangedEventHandler(
            (sender, e) =>
            {
                logProgress?.Invoke(e.ProgressPercentage);
                if (e.UserState != null)
                    logProgressText?.Invoke(e.UserState as String);
            });
            worker.DoWork +=
            (s3, e3) =>
            {
                new PLoadGameData(spd).Run();
                new PExtractAllGameData(spd).Run();
                new PEnd(spd).Run();
            };

            worker.RunWorkerAsync();
        }
    
        public static Stream? GetFileStream(string internalPath)
        {
            var path = $"{assetDatabaseDir}\\{internalPath}";

            if (File.Exists(path))
            {
                return File.OpenRead(path);
            }

            return null;
        }
    }
}
