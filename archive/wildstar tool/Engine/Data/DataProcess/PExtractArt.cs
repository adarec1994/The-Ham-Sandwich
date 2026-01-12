namespace ProjectWS.Engine.Data.DataProcess
{
    internal class PExtractArt : Process
    {
        int count = 0;
        const string TEX = ".tex";

        public PExtractArt(SharedProcessData spd) : base(spd)
        {

        }

        public override void Run()
        {
            LogProgress(0, Info());

            if (this.sharedProcessData?.gameData != null && this.sharedProcessData?.assetDBFolder != null)
            {
                Dictionary<string, Block.FileEntry> scannedFiles = new Dictionary<string, Block.FileEntry>();
                FileScan($"{Data.Archive.rootBlockName}\\Sky", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Camera", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Creature", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Dev", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\FX", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\LevelDesign", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Light", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Prop", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Sky", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\SoundEmitters", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Structure", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Terrain", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Texture_Library", this.sharedProcessData.assetDBFolder, ref scannedFiles);
                FileScan($"{Data.Archive.rootBlockName}\\Art\\Water", this.sharedProcessData.assetDBFolder, ref scannedFiles);

                count = scannedFiles.Count;

                int idx = 0;
                Parallel.ForEach(scannedFiles, new ParallelOptions() { MaxDegreeOfParallelism = 10 }, entry =>
                {
                    bool copy = true;

                    if (Path.GetExtension(entry.Key).ToLower() == TEX)
                    {
                        using (var ms = this.sharedProcessData.gameData.GetFileData(entry.Value))
                        {
                            if (ms != null)
                            {
                                FileFormats.Tex.File tex = new FileFormats.Tex.File("");
                                tex.Read(ms);

                                if (tex.header?.format == 0 && tex.header?.isCompressed == 1) // if jpeg
                                {
                                    //Console.WriteLine("CONVERT");
                                    tex.ConvertMipDataToDXT();
                                    tex.Write(entry.Key);
                                    LogProgress(idx++);
                                    copy = false;
                                }
                            }
                        }
                    }

                    if (copy)
                    {
                        //Console.WriteLine("COPY");
                        using (var fs = new System.IO.FileStream(entry.Key, FileMode.Create, System.IO.FileAccess.Write))
                        using (var ms = this.sharedProcessData.gameData.GetFileData(entry.Value))
                        {
                            ms?.WriteTo(fs);
                            LogProgress(idx++);
                        }
                    }

                });
            }
        }

        private void FileScan(string gameDir, string assetDir, ref Dictionary<string, Block.FileEntry> propFiles)
        {
            if (this.sharedProcessData?.gameData == null) return;

            var folderList = this.sharedProcessData.gameData.GetFolderList(gameDir);
            var fileEntries = this.sharedProcessData.gameData.GetFileEntries(gameDir);
            var gameDirNoRoot = gameDir.Substring(gameDir.IndexOf('\\'));
            var realDir = $"{assetDir}\\{gameDirNoRoot}";

            if (!Directory.Exists(realDir))
                Directory.CreateDirectory(realDir);

            foreach (var entry in fileEntries)
            {
                var realFilePath = Path.Combine(realDir, entry.Key);
                propFiles.Add(realFilePath, entry.Value);
            }

            foreach (var folder in folderList)
            {
                FileScan($"{gameDir}\\{folder}", assetDir, ref propFiles);
            }
        }

        public override string Info()
        {
            return "Extracting Art.";
        }

        public override int Total()
        {
            return count;
        }
    }
}
