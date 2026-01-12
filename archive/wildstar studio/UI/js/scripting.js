$('document').ready(function() {
    $('#scriptBarEntry').click(function() {
        $('#scriptEditor').css({ display: 'block' });
        editAreaLoader.init({
            id: "scriptCodeEditor",
            syntax: "js",
            start_highlight: true,
            allow_resize: false,
            font_family: 'Consolas',
            word_wrap: true,
            allow_toggle: false
        });
    });

    $('#scriptBackButton').click(function() {
        $('#scriptEditor').css({ display: 'none' });
    });

    $('#runScriptButton').click(function() {
        var code = editAreaLoader.getValue('scriptCodeEditor');
        API.runScript(code);
    });

    $('#scriptSaveButton').click(function () {
        var code = editAreaLoader.getValue('scriptCodeEditor');
        API.filesystem.saveScript(code);
    });

    $('#scriptLoadButton').click(function () {
        var code = API.filesystem.openScript();
        if (code === null) {
            return;
        }

        editAreaLoader.setValue('scriptCodeEditor', code);
        
    });
});

$.fn.selectRange = function(start, end) {
    if(!end) end = start;
    return this.each(function() {
        if (this.setSelectionRange) {
            this.focus();
            this.setSelectionRange(start, end);
        } else if (this.createTextRange) {
            var range = this.createTextRange();
            range.collapse(true);
            range.moveEnd('character', end);
            range.moveStart('character', start);
            range.select();
        }
    });
};

var logConsoleMessage = function(msg) {
    var code = '<pre class="contentText fontOverride">' + msg + '</pre>';
    var html = $('#console').html();
    html += code;
    $('#console').html(html);
    $("#console").scrollTop($("#console")[0].scrollHeight);
}

$(window).resize(function() {
    var code = editAreaLoader.getValue('scriptCodeEditor');
    $('#scriptCodeEditor').focus();
    var sel = document.getElementById('scriptCodeEditor').selectionStart;

    $('#codeBar').html('<textarea id="scriptCodeEditor"></textarea>');
    $('#scriptCodeEditor').text(code);
    editAreaLoader.init({
        id: "scriptCodeEditor",
        syntax: "js",
        start_highlight: true,
        allow_resize: false,
        font_family: 'Consolas',
        word_wrap: true,
        allow_toggle: false,
        EA_load_callback: function() {
            $('#scriptCodeEditor').focus();
            document.getElementById('scriptCodeEditor').setSelectionRange(sel, sel);
        }
    });
});