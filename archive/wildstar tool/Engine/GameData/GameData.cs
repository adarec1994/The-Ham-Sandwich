using ProjectWS.Engine.Database;
using System.IO.Compression;

namespace ProjectWS.Engine.Data
{
    public class GameData
    {
        public const string CLIENTDATA = "ClientData";

        public Engine engine;
        public ResourceManager.Manager? resourceManager;
        public string? gamePath;
        public string dataFolderPath;
        public bool dataAvailable;
        public bool databaseAvailable;
        public Dictionary<string, Archive>? archives;
        public Action<Data.GameData>? onLoaded;
        public Tables? database;

        public GameData(Engine engine, string? gamePath = null, Action<Data.GameData>? onLoaded = null)
        {
            this.resourceManager = engine.resourceManager;
            this.dataAvailable = false;
            this.databaseAvailable = false;
            this.engine = engine;
            this.onLoaded = onLoaded;

            if (gamePath == null)
            {
                // Try to auto detect where the game is installed
                string? gameExePath = FindInstallationUsingRegistry();
                if (gameExePath == null)
                {
                    this.dataAvailable = false;
                    return;
                }
                else
                {
                    gamePath = Path.GetDirectoryName(gameExePath);
                }
            }

            if (!File.Exists($"{gamePath}\\Wildstar.exe"))
            {
                Debug.Log($"Exe doesn't exist in : {gamePath}");
                this.dataAvailable = false;
                return;
            }

            this.gamePath = gamePath;
            this.dataFolderPath = $"{gamePath}\\Patch\\";
        }

        public void Read(bool loadDatabase = true, Action<int>? progress = null)
        {
            this.archives = new Dictionary<string, Archive>();

            this.archives.Add(CLIENTDATA, new Archive(this.dataFolderPath, CLIENTDATA));
            this.archives[CLIENTDATA].Read(progress);

            this.dataAvailable = true;

            // Load database
            if (loadDatabase)
                this.database = new Tables(this.engine, this);
        }

        string? FindInstallationUsingRegistry()
        {
#if WINDOWS
            var key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WildStar");
            if (key != null)
            {
                System.Object o = key.GetValue("DisplayIcon");
                if (o != null)
                {
                    string path = o as string;
                    Debug.Log("Found WildStar install location : " + path);
                    return path;
                }
            }
            Debug.LogWarning("Can't find registry key for WS install location.");
#endif
            return null;
        }

        public MemoryStream? GetFileData(Data.Block.FileEntry fileEntry, string archiveName = CLIENTDATA)
        {
            if (fileEntry == null)
                return null;

            var archive = this.archives[archiveName];
            var aarcEntry = archive.data.aarcEntries[fileEntry.hash];
            var blockHeader = archive.data.blockHeaders[aarcEntry.blockIndex];

            // Access data file
            string archivePath = $"{this.dataFolderPath}{archiveName}.archive";
            using (var fs = File.OpenRead(archivePath))
            {
                byte[] data = new byte[(long)blockHeader.blockSize];
                fs.Seek((long)blockHeader.blockOffset, SeekOrigin.Begin);
                fs.Read(data, 0, data.Length);

                switch (fileEntry.compression)
                {
                    case Block.FileEntry.Compression.Uncompressed:
                        return new MemoryStream(data);
                    case Block.FileEntry.Compression.ZLib:
                        try
                        {
                            var unpackedData = new byte[fileEntry.uncompressedSize];
                            using (MemoryStream ms = new MemoryStream(data, 2, data.Length - 2))
                            {
                                using (var deflate = new DeflateStream(ms, CompressionMode.Decompress))
                                {
                                    deflate.Read(unpackedData, 0, unpackedData.Length);
                                }
                            }
                            return new MemoryStream(unpackedData);
                        }
                        catch (Exception e)
                        {
                            Debug.LogException(e);
                            return null;
                        }
                    case Block.FileEntry.Compression.LZMA:
                        try
                        {
                            var unpackedData = new byte[fileEntry.uncompressedSize];
                            SevenZip.Compression.LZMA.Decoder decoder = new SevenZip.Compression.LZMA.Decoder();
                            var decompressed = new MemoryStream();
                            using (var compressed = new MemoryStream(data))
                            {
                                var props = new byte[5];
                                compressed.Read(props, 0, 5);
                                decoder.SetDecoderProperties(props);
                                decoder.Code(compressed, decompressed, data.Length, unpackedData.Length, null);
                                decompressed.Seek(0, SeekOrigin.Begin);
                            }
                            return decompressed;
                        }
                        catch (Exception e)
                        {
                            Debug.LogException(e);
                            return null;
                        }
                    default:
                        Debug.LogWarning($"Unsupported Compression : {fileEntry.compression}");
                        return null;
                }
            }
        }

        public MemoryStream GetFileData(string path, string archiveName = CLIENTDATA)
        {
            if (!this.archives.ContainsKey(archiveName))
            {
                Debug.LogWarning($"Archive {archiveName} is not loaded.");
                return null;
            }

            var archive = this.archives[archiveName];

            string pathCorrected = $"{Data.Archive.rootBlockName}\\{path}".ToLower();
            if (!archive.fileList.ContainsKey(pathCorrected))
            {
                Debug.LogWarning($"Archive {archiveName} : File list doesn't contain {path}");
                return null;
            }

            var fileEntry = archive.fileList[pathCorrected];

            return GetFileData(fileEntry);
        }

        public List<string> GetFolderList(string path, string archiveName = CLIENTDATA)
        {
            if (!this.archives.ContainsKey(archiveName))
            {
                Debug.LogWarning($"Archive {archiveName} is not loaded.");
                return null;
            }

            var archive = this.archives[archiveName];

            List<string> folderList = new List<string>();
            var branch = archive.blockTree[path];
            if (branch.directoryEntries != null)
            {
                foreach (Block.DirectoryEntry directoryEntry in branch.directoryEntries)
                {
                    folderList.Add(directoryEntry.name);
                }
            }
            return folderList;
        }

        public Dictionary<string, Block.FileEntry>? GetFileEntries(string path, string archiveName = CLIENTDATA)
        {
            if (this.archives == null) return null;

            if (!this.archives.ContainsKey(archiveName))
            {
                Debug.LogWarning($"Archive {archiveName} is not loaded.");
                return null;
            }

            if (path[path.Length - 1] == '\\')
                path = path.Remove(path.Length - 1, 1);

            var archive = this.archives[archiveName];

            if (archive.blockTree.TryGetValue(path, out Block? branch))
            {
                Dictionary<string, Block.FileEntry> fileList = new Dictionary<string, Block.FileEntry>();
                if (branch.fileEntries != null)
                {
                    foreach (Block.FileEntry fileEntry in branch.fileEntries)
                    {
                        fileList.Add(fileEntry.name, fileEntry);
                    }
                }
                return fileList;
            }
            else
            {
                Debug.LogWarning("BlockTree doesn't contain branch for " + path);
                return null;
            }
        }
    }
}