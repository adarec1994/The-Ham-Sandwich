var codeView = {
    show: function(fileName) {
        var fileType = API.filesystem.getExtension(fileName);
        var content;

		if(fileType == "form")
			content = API.filesystem.openTextUtf16(fileName);
		else
			content = API.filesystem.openText(fileName);

        $('#codeGlView_content').text(content);
        $('#codeGlView_content').removeClass();
        $('#codeGlView_content').addClass(fileType);
        $('#codeGlView_content').each(function (i, e) { hljs.highlightBlock(e); });

        $('#codeGlView')[0].scrollTop = 0;

        $('#codeGlView').css({ 'display': 'block' });
    },

    hide: function() {
        $('#codeGlView').css({ 'display': 'none' });
    }
};

viewController.extensionMap.xml = codeView;
viewController.extensionMap.lua = codeView;
viewController.extensionMap.form = codeView;