var textureView = {
    textureObject: null,

    show: function(texName) {
        this.setObject(texName);
        $('#textureGlView').css({ 'display': 'block' });
    },

    hide: function() {
        $('#textureGlView').css({ 'display': 'none' });
    },

    setObject: function(texObj) {
        this.textureObject = texObj;
        $('#textureNameDesc').text(texObj);

        var propObj = { };
        API.filesystem.openTexture(texObj, this, propObj);
    },

    onPropertiesLoaded: function(props) {
        $('#textureSizeDesc').text(props.width + "x" + props.height);
        $('#textureTypeDesc').text(props.formatEntry);
    }
}

viewController.extensionMap.tex = textureView;

$('document').ready(function() {
    $('#exportTextureButton').click(function() {
        API.filesystem.exportTexture();
    });

    $('#resetImageButton').click(function() {
        API.resetImage();
    });
});