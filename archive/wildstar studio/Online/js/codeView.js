var codeView = {
    show: function(fileName) {
        var fileType = API.filesystem.getExtension(fileName);
        $('#glDrawArea').css({ display: 'none' });

        API.filesystem.openText(fileName, (function (fileType) {
            return function (content) {
                $('#codeGlView_content').text(content);
                $('#codeGlView_content').removeClass();
                $('#codeGlView_content').addClass(fileType);
                $('#codeGlView_content').each(function (i, e) { hljs.highlightBlock(e); });

                $('#codeGlView').css({ 'display': 'block' });
            };
        })(fileType));
    },

    hide: function() {
        $('#codeGlView').css({ 'display': 'none' });
    },

    extract: function (fileName) {
        var fileType = API.filesystem.getExtension(fileName);
        $('#glDrawArea').css({ display: 'none' });

        API.filesystem.openText(fileName,
            (function(filename) { 
                return function (content) {
                    filename = filename.replace(/^.*[\\\/]/, '')
                    var blob = new Blob([content], { type: 'text/plain' });
                    var url = window.URL.createObjectURL(blob);
                    var a = document.createElement('a');
                    a.setAttribute('href', url);
                    a.setAttribute('download', filename);
                    var event = document.createEvent('MouseEvents');
                    event.initMouseEvent('click', true, true, window, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
                    a.dispatchEvent(event);
                };
            })(fileName));
    }
};

viewController.extensionMap.xml = codeView;
viewController.extensionMap.lua = codeView;
viewController.extensionMap.form = codeView;