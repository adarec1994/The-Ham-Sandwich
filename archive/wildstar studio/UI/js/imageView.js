var imageView = {
    show: function(fileName) {
        $('#imageContainer').attr('src', 'asset://local/.filesys/get_image?path=' + encodeURIComponent(fileName));
        //this.textureObject.setAttr('src', 'asset://.filesys/get_image?path=' + encodeURIComponent(fileName));
        $('#imageView').css({ 'display': 'block' });
	},
	
    hide: function() {
        $('#imageView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.jpg = imageView;
viewController.extensionMap.bmp = imageView;