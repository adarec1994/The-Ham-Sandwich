var imageView = {
    mimeTypes: {
        'jpg': 'image/jpg',
        'png': 'image/png',
        'bmp': 'image/bmp'
    },

    show: function (fileName) {
        $('#glDrawArea').css({ display: 'none' });
        var binFile = new BinFile(fileName);
        binFile.onload = function () {
            var ext = API.filesystem.getExtension(this.filePath).toLowerCase();
            if (ext in imageView.mimeTypes) {
                var blob = new Blob([this.readBytes(this.fileSize).buffer], { type: imageView.mimeTypes[ext] });
                var url = window.URL.createObjectURL(blob);
                $('#imageContainer').attr('src', url);
                $('#imageView').css({ 'display': 'block' });
            }
        };

        binFile.open();
	},
	
    hide: function() {
        $('#imageView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.jpg = imageView;
viewController.extensionMap.bmp = imageView;
viewController.extensionMap.png = imageView;