using ProjectWS.Engine.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Windows.Documents;

namespace ProjectWS.Engine.Project
{
    public class ProjectManager
    {
        public static Project? project;
        public static string? projectFile;

        public const string PROJECT_EXTENSION = "wsProject";

        public static void LoadProject(string? path)
        {
            if (path == null || path == String.Empty)
                return;

            if (File.Exists(path))
            {
                string? jsonString = File.ReadAllText(path);
                if (jsonString != null && jsonString != String.Empty)
                {
                    project = JsonSerializer.Deserialize<Project>(jsonString);
                    projectFile = path;
                    Engine.settings.projectManager.previousLoadedProject = path;
                    SettingsSerializer.Save();
                    Debug.Log("Loaded Project : " + project?.Name);
                }
            }
        }

        public static void CreateProject(string path)
        {
            project = new Project();
            project.Name = Path.GetFileNameWithoutExtension(path);
            project.UUID = Guid.NewGuid();
            projectFile = path;

            // Find last world ID
            uint lastWorldID = 0;
            foreach (var item in DataManager.database.world.records)
            {
                if (item.Key > lastWorldID)
                    lastWorldID = item.Key;
            }
            project.lastWorldID = lastWorldID;

            // Find last worldLocation2 ID
            uint lastWorldLocationID = 0;
            foreach (var item in DataManager.database.worldLocation.records)
            {
                if (item.Key > lastWorldLocationID)
                    lastWorldLocationID = item.Key;
            }
            project.lastWorldLocationID = lastWorldLocationID;

            SaveProject();

            Engine.settings.projectManager.previousLoadedProject = path;
            SettingsSerializer.Save();

            Debug.Log("Created new Project : " + project.Name);
        }

        public static void SaveProject()
        {
            if (projectFile == null || projectFile == String.Empty)
                return;

            if (project == null)
                return;

            var options = new JsonSerializerOptions { WriteIndented = true };
            string data = JsonSerializer.Serialize(project, options);
            File.WriteAllText(projectFile, data);

            // TODO: Save all map changes here
        }

        public static List<Project.Map>? GetMapList()
        {
            return project?.Maps;
        }
    }
}
