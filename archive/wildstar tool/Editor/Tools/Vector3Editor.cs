using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;

namespace ProjectWS.Editor.Tools
{
    public partial class PropTool
    {
        //Custom editors that are used as attributes MUST implement the ITypeEditor interface.
        public class Vector3Editor : Xceed.Wpf.Toolkit.PropertyGrid.Editors.ITypeEditor
        {
            public FrameworkElement ResolveEditor(Xceed.Wpf.Toolkit.PropertyGrid.PropertyItem propertyItem)
            {
                StackPanel stackPanel = new StackPanel();
                stackPanel.Orientation = Orientation.Horizontal;

                TextBox textBoxX = MakeTextbox("Value[0]", propertyItem);
                stackPanel.Children.Add(textBoxX);
                TextBox textBoxY = MakeTextbox("Value[1]", propertyItem);
                stackPanel.Children.Add(textBoxY);
                TextBox textBoxZ = MakeTextbox("Value[2]", propertyItem);
                stackPanel.Children.Add(textBoxZ);

                return stackPanel;
            }

            TextBox MakeTextbox(string bindingName, Xceed.Wpf.Toolkit.PropertyGrid.PropertyItem propertyItem)
            {
                TextBox textBox = new TextBox();
                textBox.IsReadOnly = false;
                textBox.Width = 70;

                //textBox.TextChanged += propertyItem.PropertyChanged
                //create the binding from the bound property item to the editor
                var _binding = new Binding(bindingName); //bind to the Value property of the PropertyItem
                _binding.Source = propertyItem;
                _binding.ValidatesOnExceptions = true;
                _binding.ValidatesOnDataErrors = true;
                _binding.Mode = propertyItem.IsReadOnly ? BindingMode.OneWay : BindingMode.TwoWay;
                BindingOperations.SetBinding(textBox, TextBox.TextProperty, _binding);

                return textBox;
            }
        }
    }
}
