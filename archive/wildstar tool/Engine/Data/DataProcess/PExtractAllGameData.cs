namespace ProjectWS.Engine.Data.DataProcess
{
    internal class PExtractAllGameData : Process
    {
        int count = 0;

        public PExtractAllGameData(SharedProcessData spd) : base(spd)
        {

        }

        public override void Run()
        {
            LogProgress(0, Info());

            if (this.sharedProcessData?.gameData != null && this.sharedProcessData?.extractPath != null)
            {
                Dictionary<string, Block.FileEntry> scannedFiles = new Dictionary<string, Block.FileEntry>();
                FileScan(Data.Archive.rootBlockName, this.sharedProcessData.extractPath, ref scannedFiles);

                count = scannedFiles.Count;

                int idx = 0;
                Parallel.ForEach(scannedFiles, new ParallelOptions() { MaxDegreeOfParallelism = 10 }, entry =>
                {
                    if (File.Exists(entry.Key))
                    {
                        LogProgress(idx++);
                        return;
                    }

                    using (var fs = new System.IO.FileStream(entry.Key, FileMode.Create, System.IO.FileAccess.Write))
                    using (var ms = this.sharedProcessData.gameData.GetFileData(entry.Value))
                    {
                        ms?.WriteTo(fs);
                        LogProgress(idx++);
                    }
                });
            }
        }

        private void FileScan(string gameDir, string assetDir, ref Dictionary<string, Block.FileEntry> propFiles)
        {
            if (this.sharedProcessData?.gameData == null) return;

            var folderList = this.sharedProcessData.gameData.GetFolderList(gameDir);
            var fileEntries = this.sharedProcessData.gameData.GetFileEntries(gameDir);
            string gameDirNoRoot;
            if (gameDir.Contains('\\'))
                gameDirNoRoot = gameDir.Substring(gameDir.IndexOf('\\'));
            else
                gameDirNoRoot = "";
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
            return "Extracting Game Data.";
        }

        public override int Total()
        {
            return count;
        }
    }
}
