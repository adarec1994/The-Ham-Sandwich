using AvalonDock.Layout;
using OpenTK.Wpf;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Editor
{
    public class Program
    {
        public static Editor? editor;
        public static App? app;
        public static MainWindow? mainWindow;

        [STAThread]
        static void Main(string[] args)
        {
            app = new App();
            editor = new Editor();
            app.InitializeComponent();
            app.Run();
        }
    }
}
