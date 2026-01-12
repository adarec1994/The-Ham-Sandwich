using ProjectWS.Engine;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace ProjectWS.Engine
{
    public static class SettingsSerializer
    {
        public static void Save()
        {
            var options = new JsonSerializerOptions { WriteIndented = true };
            string data = JsonSerializer.Serialize(Engine.settings, options);
            File.WriteAllText("Settings.json", data);
        }

        public static void Load()
        {
            if (File.Exists("Settings.json"))
            {
                string? jsonString = File.ReadAllText("Settings.json");
                if (jsonString != null && jsonString != String.Empty)
                {
                    Engine.settings = JsonSerializer.Deserialize<Settings>(jsonString);
                }
            }
        }
    }
}
