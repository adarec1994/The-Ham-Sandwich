var viewController = {
    activeView: null,
    extensionMap: { },

    switchTo: function(newView, data) {
        if(newView != this.activeView && this.activeView != null && this.activeView.hide != null) {
            this.activeView.hide();
        }

        if(newView != null) {
            if(data != null)
                newView.show(data);
            else
                newView.show();
        }
        this.activeView = newView;
    },

    onFilePress: function(ext, data) {
        var dlg = this.extensionMap[ext];
        if(dlg != null) {
            try {
                this.switchTo(dlg, data);
            } catch (e) {
                console.log(e + "\n" + e.stack);
            }
        }
    },

    extract: function (entry) {
        var ext = API.filesystem.getExtension(entry);

        if (ext in this.extensionMap) {
            var dlg = this.extensionMap[ext];
            if (dlg.extract) {
                dlg.extract(entry);
            }
        }
    }
};

var API = {
    extractionPath: '',
    exportTextureAsBmp: false,
    exportTblAsCsv: false,
    mouseSensitivity: 1.0,
    dataFolder: null,
    renderer: null,

    filesystem: {
        setExtractionFolder: function (path) {
            API.extractionPath = path;
            window.localStorage['API.extractionPath'] = path;
        },

        getExtractionFolder: function () {
            return API.extractionPath;
        },

        textureAsBmp: function () {
            return API.exportTextureAsBmp;
        },

        setTextureAsBmp: function(enabled) {
            API.exportTextureAsBmp = enabled;
            window.localStorage['API.exportTextureAsBmp'] = enabled;
        },

        setTblAsCsv: function(enabled) {
            API.exportTblAsCsv = enabled;
            window.localStorage['API.exportTblAsCsv'] = enabled;
        },

        tblAsCsv: function () {
            return API.exportTblAsCsv;
        },

        setMouseSensitivity: function (value) {
            API.mouseSensitivity = value;
            window.localStorage['API.mouseSensitivity'] = value;
        },

        setPath: function () {

        },

        getPath: function () {

        },

        getExtension: function (value) {
            var match = value.match(/\.([0-9a-z]+)$/i);
            if (match.length <= 1) {
                return null;
            }

            return match[1];
        },

        openText: function (name, success) {
            var file = new BinFile(name);
            file.onload = (function (file, callback) {
                return function () {
                    var bytes = file.readBytes(file.fileSize);
                    var blob = new Blob([bytes.buffer], { type: 'application/octet-stream' });
                    var reader = new FileReader();
                    reader.onload = (function (callback) {
                        return function (event) {
                            callback(event.target.result);
                        };
                    })(callback);

                    reader.readAsBinaryString(blob);
                };
            })(file, success);

            file.open();
        }
    },

    initialize: function () {
        var path = window.localStorage['API.extractionPath'];
        if (path != null) {
            API.filesystem.setExtractionFolder(path);
        }

        API.filesystem.setTextureAsBmp(window.localStorage['API.exportTextureAsBmp'] || false);
        API.filesystem.setTblAsCsv(window.localStorage['API.exportTblAsCsv'] || false);

        if (window.File == null || window.FileReader == null || window.FileList == null || window.Blob == null) {
            alert('Sorry, file api not supported in your browser. You may get Chrome, Firefox or the latest version of Internet Explorer to use this tool.');
        }

        $(document).ready(function () {
            API.renderer = new GxContext(document.getElementById('glDrawArea'));
        })
    }
};

API.initialize();